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

#include "brpc.h"
#include "config.pb.h"
#include "unified_scheduler_manager.h"
#include "utils.h"
#include "global.h"
#include "thread_data.h"
#include "rapidjson/pointer.h"

namespace uskit {

UnifiedSchedulerManager::UnifiedSchedulerManager() {}

UnifiedSchedulerManager::~UnifiedSchedulerManager() {}

int UnifiedSchedulerManager::init(const UnifiedSchedulerConfig& config) {
    // Register global functions and policies
    register_function();
    register_policy();

    // Load unified schedulers that are specified in configuration.
    for (int i = 0; i < config.load_size(); ++i) {
        const std::string& usid = config.load(i);
        UnifiedScheduler us;
        if (us.init(config.root_dir(), usid) != 0) {
            LOG(ERROR) << "Failed to init app [" << usid << "]";
            return -1;
        }
        _us_map.emplace(usid, std::move(us));
    }

    if (config.required_params_size() == 0) {
        _required_params = {"logid", "uuid", "usid", "query"};
    }
    for (int i = 0; i < config.required_params_size(); ++i) {
        const UnifiedSchedulerConfig_RequiredParam required_param = config.required_params(i);
        std::string param_name = required_param.param_name();
        _required_params.push_back(param_name);
        if (required_param.has_default_value()) {
            _params_default.emplace(param_name, required_param.default_value());
        } else if (required_param.has_param_path()) {
            _params_path.emplace(param_name, required_param.param_path());
        } else {
            LOG(ERROR) << "Failed to init uskit with required parameter [" << param_name << "]";
            return -1;
        }
    }
    return 0;
}

int UnifiedSchedulerManager::run(BRPC_NAMESPACE::Controller* cntl) {
    Timer parse_request_tm("parse_request_t_ms");
    parse_request_tm.start();

    USRequest request;

    // Parse user request.
    if (parse_request(cntl, request) != 0) {
        parse_request_tm.stop();
        return -1;
    }
    parse_request_tm.stop();
    LOG(INFO) << "REQUEST: " << json_encode(request);
    // Run unified scheduler of specific usid.
    std::string usid = request["usid"].GetString();
    auto us_iter = _us_map.find(usid);

    if (us_iter != _us_map.end()) {
        USResponse response(rapidjson::kObjectType);
        if (us_iter->second.run(request, response) != 0) {
            send_response(cntl, nullptr, ErrorCode::INTERNAL_SERVER_ERROR);
            return -1;
        }
        send_response(cntl, &response);
    } else {
        send_response(cntl, nullptr, ErrorCode::USID_NOT_FOUND);
        return -1;
    }

    return 0;
}

int UnifiedSchedulerManager::parse_request(BRPC_NAMESPACE::Controller* cntl, USRequest& request) {
    std::string request_json = cntl->request_attachment().to_string();
    // Parse request parameters from JSON.
    if (request.Parse(request_json.c_str()).HasParseError() || !request.IsObject()) {
        send_response(cntl, nullptr, ErrorCode::INVALID_JSON);
        return -1;
    }

    for (auto header_iter = cntl->http_request().HeaderBegin();
         header_iter != cntl->http_request().HeaderEnd();
         ++header_iter) {
        std::string path = "/__HEADER__/" + header_iter->first;
        std::string header_val = header_iter->second;
        rapidjson::Pointer(path.c_str()).Set(request, header_val.c_str());
    }

    // Check required parameters
    UnifiedSchedulerThreadData* td =
            static_cast<UnifiedSchedulerThreadData*>(BRPC_NAMESPACE::thread_local_data());
    for (auto& param : _required_params) {
        std::string value = "";
        auto _params_default_iter = _params_default.find(param.c_str());
        auto _params_value_path_iter = _params_path.find(param.c_str());
        if (_params_default_iter != _params_default.end()) {
            value = _params_default_iter->second;
        } else if (_params_value_path_iter != _params_path.end()) {
            std::string path = _params_value_path_iter->second;
            if (path[0] != '/') {
                path = "/" + path;
            }
            // Root path.
            if (path == "/") {
                path = "";
            }
            rapidjson::Pointer pointer(path.c_str());
            rapidjson::Value* req_value = rapidjson::GetValueByPointer(request, pointer);
            if (req_value == nullptr) {
                send_response(
                        cntl,
                        nullptr,
                        ErrorCode::MISSING_PARAM,
                        ErrorMessage.at(ErrorCode::MISSING_PARAM) + ": " + param +
                                ", supposed to be at " + path);
                return -1;
            } else {
                value = json_encode(*req_value);
            }

        } else if (!request.HasMember(param.c_str())) {
            send_response(
                    cntl,
                    nullptr,
                    ErrorCode::MISSING_PARAM,
                    ErrorMessage.at(ErrorCode::MISSING_PARAM) + ": " + param);
            return -1;
        } else if (!request[param.c_str()].IsString()) {
            send_response(
                    cntl,
                    nullptr,
                    ErrorCode::INVALID_JSON,
                    ErrorMessage.at(ErrorCode::INVALID_JSON) + ": " + param);
            return -1;
        } else {
            value = request[param.c_str()].GetString();
        }
        if (param == "logid") {
            // Setup logid of this request.
            td->set_logid(value);
        } else {
            td->add_log_entry(param, value);
        }
        rapidjson::Value tmp_key(param.c_str(), request.GetAllocator());
        rapidjson::Value tmp_value(value.c_str(), request.GetAllocator());
        if (request.HasMember(param.c_str())) {
            request.EraseMember(param.c_str());
        }
        request.AddMember(tmp_key, tmp_value, request.GetAllocator());
    }

    return 0;
}

int UnifiedSchedulerManager::send_response(
        BRPC_NAMESPACE::Controller* cntl,
        USResponse* response,
        ErrorCode error_code,
        const std::string& error_msg) {
    cntl->http_response().set_content_type("application/json;charset=UTF-8");
    int http_status_code = 200;
    if (error_code != 0) {
        http_status_code = error_code / 1000 * 100;
    }
    // Setup HTTP status.
    cntl->http_response().set_status_code(http_status_code);

    rapidjson::Document final_response;
    final_response.SetObject();
    rapidjson::Document::AllocatorType& allocator = final_response.GetAllocator();
    if (response != nullptr && response->HasMember("error_code")) {
        if ((*response)["error_code"].IsString()) {
            final_response.AddMember(
                    "error_code", std::atoi((*response)["error_code"].GetString()), allocator);
        } else if ((*response)["error_code"].IsInt()) {
            final_response.AddMember("error_code", (*response)["error_code"].GetInt(), allocator);
        } else {
            final_response.AddMember("error_code", error_code, allocator);
        }
    } else {
        final_response.AddMember("error_code", error_code, allocator);
    }

    rapidjson::Value error_msg_value;
    if (response != nullptr && response->HasMember("error_msg") &&
        (*response)["error_msg"].IsString()) {
        error_msg_value.SetString((*response)["error_msg"].GetString(), allocator);
    } else if (error_msg.empty()) {
        error_msg_value.SetString(
                ErrorMessage.at(error_code).c_str(),
                ErrorMessage.at(error_code).length(),
                allocator);
    } else {
        error_msg_value.SetString(error_msg.c_str(), error_msg.length(), allocator);
    }
    final_response.AddMember("error_msg", error_msg_value, allocator);

    if (response != nullptr) {
        final_response.AddMember("result", *response, allocator);
    }

    std::string final_response_json = json_encode(final_response);
    cntl->response_attachment().append(final_response_json);

    return 0;
}

}  // namespace uskit
