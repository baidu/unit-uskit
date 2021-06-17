// Copyright (c) 2018 Baidu, Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "policy/backend/dynamic_policy.h"
#include "utils.h"
#include <rapidjson/pointer.h>

namespace uskit {
namespace policy {
namespace backend {

int DynamicHttpRequestPolicy::init(const RequestConfig& config, const Backend* backend) {
    _channel = backend->channel();
    const BackendRequestConfig* template_config = nullptr;
    if (config.has_include()) {
        template_config = backend->request_config(config.include());
    }
    if (_request_config.init(config, template_config) != 0) {
        LOG(ERROR) << "Failed to init HTTP request config";
        return -1;
    }

    return 0;
}

int DynamicHttpRequestPolicy::run(BackendController* cntl) const {
    DynamicHTTPController* dyn_http_cntl = static_cast<DynamicHTTPController*>(cntl);
    expression::ExpressionContext request_context("request block", cntl->context());
    // Generate request config dynamically.
    if (_request_config.run(request_context) != 0) {
        US_LOG(ERROR) << "Failed to generate HTTP request config";
        return -1;
    }
    US_DLOG(INFO) << "Generated HTTP request config: " << request_context.str();

    rapidjson::Value* dynamic_args_node = request_context.get_variable("dynamic_args_node");
    if (dynamic_args_node == nullptr) {
        US_LOG(ERROR) << "Required dynamic_args_node option";
        return -1;
    }
    if (!dynamic_args_node->IsString()) {
        US_LOG(ERROR) << "Required dynamic_args_node to be string, "
                      << uskit::get_value_type(*dynamic_args_node) << " were given";
        return -1;
    }
    rapidjson::Value* http_method = request_context.get_variable("http_method");
    if (http_method == nullptr) {
        US_LOG(ERROR) << "Required HTTP method";
        return -1;
    }

    rapidjson::Value* http_uri = request_context.get_variable("http_uri");
    if (http_uri == nullptr) {
        US_LOG(ERROR) << "Required HTTP URI";
        return -1;
    }
    rapidjson::Value* dynamic_args = request_context.get_variable("dynamic_args");
    if (dynamic_args == nullptr) {
        US_LOG(ERROR) << "Required dynamic_args option";
        return -1;
    }
    for (auto& dynamic_ele : dynamic_args->GetObject()) {
        if (dynamic_ele.value.IsNull()) {
            continue;
        }
        if (!dynamic_ele.value.IsArray()) {
            US_LOG(ERROR) << "Required dynamic_args to be array, "
                          << uskit::get_value_type(dynamic_ele.value) << " were given";
            return -1;
        }
        // For each element in dynamic args, we build a new brpc controller and push back at the
        // end.
        std::lock_guard<std::mutex> lock(dyn_http_cntl->_outer_mutex);
        for (size_t index = 0; index < dynamic_ele.value.GetArray().Size(); ++index) {
            std::unique_ptr<BRPC_NAMESPACE::Controller> brpc_cntl(new BRPC_NAMESPACE::Controller());
            std::string dynamic_ele_str;
            if (dynamic_ele.value[index].IsString()) {
                dynamic_ele_str = dynamic_ele.value[index].GetString();
            } else {
                dynamic_ele_str = json_encode(dynamic_ele.value[index]);
            }
            // Set HTTP Headers
            rapidjson::Value* http_header = request_context.get_variable("http_header");
            if (http_header != nullptr) {
                std::string content_type_key("Content-Type");
                for (auto& m : http_header->GetObject()) {
                    if (m.value.IsNull()) {
                        continue;
                    }
                    std::string value;
                    if (m.value.IsString()) {
                        value = m.value.GetString();
                    } else {
                        value = json_encode(m.value);
                    }
                    if (m.name.GetString() == content_type_key) {
                        brpc_cntl->http_request().set_content_type(value);
                    } else {
                        brpc_cntl->http_request().SetHeader(m.name.GetString(), value);
                    }
                }
            }
            // Set HTTP Method is POST
            if (http_method->GetString() == std::string("post")) {
                brpc_cntl->http_request().set_method(BRPC_NAMESPACE::HTTP_METHOD_POST);
            }
            // Case args node is uri:
            if (dynamic_args_node->GetString() == std::string("uri")) {
                brpc_cntl->http_request().uri() = http_uri->GetString() + dynamic_ele_str;
            } else {
                brpc_cntl->http_request().uri() = http_uri->GetString();
            }
            // Set HTTP query
            rapidjson::Value* http_query = request_context.get_variable("http_query");
            if (http_query != nullptr) {
                for (auto& m : http_query->GetObject()) {
                    if (m.value.IsNull()) {
                        continue;
                    }
                    std::string value;
                    if (m.value.IsString()) {
                        value = m.value.GetString();
                    } else {
                        value = json_encode(m.value);
                    }
                    brpc_cntl->http_request().uri().SetQuery(m.name.GetString(), value);
                }
            }
            if (dynamic_args_node->GetString() == std::string("query")) {
                brpc_cntl->http_request().uri().SetQuery(
                        dynamic_ele.name.GetString(), dynamic_ele_str);
            }
            // Set HTTP Body
            // Only support JSON format
            rapidjson::Value* http_body = request_context.get_variable("http_body");
            if (dynamic_args_node->GetString() == std::string("body")) {
                rapidjson::Value* dynamic_args_path =
                        request_context.get_variable("dynamic_args_path");
                if (dynamic_args_path == nullptr) {
                    US_LOG(ERROR) << "Required dynamic_args_path option";
                    return -1;
                }
                if (!dynamic_args_path->IsString()) {
                    US_LOG(ERROR) << "Required dynamic_args_path to be string, "
                                  << uskit::get_value_type(*dynamic_args_path) << " were given";
                    return -1;
                }
                rapidjson::Document http_body_doc(&request_context.allocator());

                if (http_body != nullptr) {
                    rapidjson::Value copyvalue(*http_body, request_context.allocator());
                    json_set_value_by_path("/", http_body_doc, copyvalue);
                }
                US_DLOG(INFO) << "path: " << dynamic_args_path->GetString();
                json_set_value_by_path(
                        dynamic_args_path->GetString(), http_body_doc, dynamic_ele.value[index]);
                const std::string& content_type = brpc_cntl->http_request().content_type();
                if (content_type.find("application/json") != std::string::npos) {
                    brpc_cntl->request_attachment().append(json_encode(http_body_doc));
                }
            } else if (http_body != nullptr) {
                const std::string& content_type = brpc_cntl->http_request().content_type();
                if (content_type.find("application/json") != std::string::npos) {
                    brpc_cntl->request_attachment().append(json_encode(*http_body));
                }
            }
            _channel->CallMethod(nullptr, brpc_cntl.get(), nullptr, nullptr, cntl->_done.get());
            dyn_http_cntl->brpc_controller_list().push_back(std::move(brpc_cntl));
        }
        return 0;
    }
    return 0;
}

int DynamicHttpResponsePolicy::init(const ResponseConfig& config, const Backend* backend) {
    const BackendResponseConfig* template_config = nullptr;
    if (config.has_include()) {
        template_config = backend->response_config(config.include());
    }
    if (_response_config.init(config, template_config) != 0) {
        LOG(ERROR) << "Failed to init HTTP response config";
        return -1;
    }

    return 0;
}

int DynamicHttpResponsePolicy::run(BackendController* cntl) const {
    DynamicHTTPController* dyn_http_cntl = static_cast<DynamicHTTPController*>(cntl);
    BackendResponse& response = cntl->response();
    expression::ExpressionContext response_context(
            "response block " + cntl->service_name(), cntl->context());
    response.SetArray();
    rapidjson::Document tmp_doc;
    for (size_t index = 0; index != dyn_http_cntl->brpc_controller_list().size(); ++index) {
        BRPC_NAMESPACE::Controller* brpc_cntl = dyn_http_cntl->brpc_controller_list()[index].get();
        // Generate response config dynamically.
        const std::string& content_type = brpc_cntl->http_response().content_type();
        // JSON Response
        US_DLOG(INFO) << "Response: " << brpc_cntl->response_attachment();
        if (content_type.find("application/json") != std::string::npos) {
            tmp_doc.Parse(brpc_cntl->response_attachment().to_string().c_str());
            if (tmp_doc.HasParseError()) {
                return -1;
            }
            response.PushBack(tmp_doc, response.GetAllocator());
        } else {
            std::string raw_response = brpc_cntl->response_attachment().to_string();
            response.PushBack(
                    rapidjson::Value().SetString(
                            raw_response.c_str(), raw_response.length(), response.GetAllocator()),
                    response.GetAllocator());
        }
    }
    response_context.set_variable("response", response);

    if (_response_config.run(response_context) != 0) {
        US_LOG(WARNING) << "Failed to generate HTTP response config";
        return -1;
    }

    US_DLOG(INFO) << "Generated respone config: " << response_context.str();  // CORE
    rapidjson::Value* output = response_context.get_variable("output");
    if (output == nullptr) {
        US_DLOG(WARNING) << "Not output found";
        return -1;
    }
    output->Swap(response);

    return 0;
}

}  // namespace backend
}  // namespace policy
}  // namespace uskit
