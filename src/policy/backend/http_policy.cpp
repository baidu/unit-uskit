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

#include "policy/backend/http_policy.h"
#include "utils.h"

namespace uskit {
namespace policy {
namespace backend {

int HttpRequestPolicy::init(const RequestConfig& config, const Backend* backend) {
    _backend = backend;
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

int HttpRequestPolicy::run(BackendController* cntl) const {
    brpc::Controller& brpc_cntl = cntl->brpc_controller();
    expression::ExpressionContext request_context("request block", cntl->context());

    // Generate request config dynamically.
    if (_request_config.run(request_context) != 0) {
        US_LOG(ERROR) << "Failed to generate HTTP request config";
        return -1;
    }
    US_DLOG(INFO) << "Generated HTTP request config: " << request_context.str();

    rapidjson::Value* http_uri = request_context.get_variable("http_uri");
    if (http_uri == nullptr) {
        US_LOG(ERROR) << "Required HTTP URI";
        return -1;
    } else {
        brpc_cntl.http_request().uri() = http_uri->GetString();
    }

    rapidjson::Value* http_method = request_context.get_variable("http_method");
    if (http_method == nullptr) {
        US_LOG(ERROR) << "Required HTTP method";
        return -1;
    } else if (http_method->GetString() == std::string("post")) {
        brpc_cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    }

    // Set HTTP Headers
    rapidjson::Value* http_header = request_context.get_variable("http_header");
    if (http_header != nullptr) {
        std::string content_type_key("Content-Type");
        for (auto & m : http_header->GetObject()) {
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
                brpc_cntl.http_request().set_content_type(value);
            } else {
                brpc_cntl.http_request().SetHeader(m.name.GetString(), value);
            }
        }
    }

    // Set HTTP Query
    rapidjson::Value* http_query = request_context.get_variable("http_query");
    if (http_query != nullptr) {
        for (auto & m : http_query->GetObject()) {
            if (m.value.IsNull()) {
                continue;
            }
            std::string value;
            if (m.value.IsString()) {
                value = m.value.GetString();
            } else {
                value = json_encode(m.value);
            }
            brpc_cntl.http_request().uri().SetQuery(m.name.GetString(), value);
        }
    }

    // Set HTTP Body
    // Only support JSON format.
    rapidjson::Value* http_body = request_context.get_variable("http_body");
    if (http_body != nullptr) {
        const std::string& content_type = brpc_cntl.http_request().content_type();
        if (content_type.find("application/json") != std::string::npos) {
            brpc_cntl.request_attachment().append(json_encode(*http_body));
        }
    }

    brpc::Channel* channel = _backend->channel();
    channel->CallMethod(nullptr, &brpc_cntl, nullptr, nullptr, brpc::DoNothing());

    return 0;
}

int HttpResponsePolicy::init(const ResponseConfig& config, const Backend* backend) {
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

int HttpResponsePolicy::run(BackendController* cntl) const {
    BackendResponse& response = cntl->response();
    brpc::Controller& brpc_cntl = cntl->brpc_controller();
    expression::ExpressionContext response_context("response block", cntl->context());
    // Generate response config dynamically.
    const std::string& content_type = brpc_cntl.http_response().content_type();

    // JSON Response
    if (content_type.find("application/json") != std::string::npos) {
        response.Parse(brpc_cntl.response_attachment().to_string().c_str());
        if (response.HasParseError()) {
            return -1;
        }
    } else {
        std::string raw_response = brpc_cntl.response_attachment().to_string();
        response.SetString(raw_response.c_str(), raw_response.length(), response.GetAllocator());
    }
    US_DLOG(INFO) << "Response: " << brpc_cntl.response_attachment();
    response_context.set_variable("response", response);

    if (_response_config.run(response_context) != 0) {
        US_LOG(ERROR) << "Failed to generate HTTP response config";
        return -1;
    }
    response_context.erase_variable("response");

    US_DLOG(INFO) << "Generated respone config: " << response_context.str();
    rapidjson::Value* output = response_context.get_variable("output");
    if (output == nullptr) {
        US_DLOG(ERROR) << "Not output found";
        return -1;
    }
    output->Swap(response);

    return 0;
}

} // namespace backend
} // namespace policy
} // namespace uskit
