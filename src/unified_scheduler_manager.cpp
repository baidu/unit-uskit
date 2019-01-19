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

#include <brpc/server.h>
#include "unified_scheduler_manager.h"
#include "utils.h"
#include "global.h"
#include "thread_data.h"

namespace uskit {

UnifiedSchedulerManager::UnifiedSchedulerManager() {
}

UnifiedSchedulerManager::~UnifiedSchedulerManager() {
}

int UnifiedSchedulerManager::init(const UnifiedSchedulerConfig &config) {
    // Register global functions and policies
    register_function();
    register_policy();

    // Load unified schedulers that are specified in configuration.
    for (int i = 0; i < config.load_size(); ++i) {
        const std::string &usid = config.load(i);
        UnifiedScheduler us;
        if (us.init(config.root_dir(), usid) != 0) {
            LOG(ERROR) << "Failed to init app [" << usid << "]";
            return -1;
        }
        _us_map.emplace(usid, std::move(us));
    }

    return 0;
}

int UnifiedSchedulerManager::run(brpc::Controller* cntl) {
    Timer parse_request_tm("parse_request_t_ms");
    parse_request_tm.start();

    USRequest request;

    // Parse user request.
    if (parse_request(cntl, request) != 0) {
        parse_request_tm.stop();
        return -1;
    }
    parse_request_tm.stop();

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

int UnifiedSchedulerManager::parse_request(brpc::Controller* cntl, USRequest& request) {
    std::string request_json = cntl->request_attachment().to_string();
    // Parse request parameters from JSON.
    if (request.Parse(request_json.c_str()).HasParseError() || !request.IsObject()) {
        send_response(cntl, nullptr, ErrorCode::INVALID_JSON);
        return -1;
    }

    // Check required parameters
    UnifiedSchedulerThreadData* td = static_cast<UnifiedSchedulerThreadData*>(brpc::thread_local_data());
    std::vector<std::string> required_params = {"logid", "uuid", "usid", "query"};
    for (auto & param : required_params) {
        if (!request.HasMember(param.c_str())) {
            send_response(cntl, nullptr, ErrorCode::MISSING_PARAM, 
                    ErrorMessage.at(ErrorCode::MISSING_PARAM) + ": " + param);
            return -1;
        } else if (!request[param.c_str()].IsString()) {
            send_response(cntl, nullptr, ErrorCode::INVALID_JSON,
                    ErrorMessage.at(ErrorCode::INVALID_JSON) + ": " + param);
            return -1;
        }        
        std::string value = request[param.c_str()].GetString();
        if (param == "logid") {
            // Setup logid of this request.
            td->set_logid(value);
        } else {
            td->add_log_entry(param, value);
        }
    }

    return 0;
}

int UnifiedSchedulerManager::send_response(brpc::Controller* cntl, USResponse* response,
                                    ErrorCode error_code, const std::string& error_msg) {
    cntl->http_response().set_content_type("application/json;charset=UTF-8");
    int http_status_code = 200;
    if (error_code != 0) {
        http_status_code = error_code / 1000 * 100;
    }
    // Setup HTTP status.
    cntl->http_response().set_status_code(http_status_code);

    rapidjson::Document final_response;
    final_response.SetObject();
    rapidjson::Document::AllocatorType& allocator =
        final_response.GetAllocator();

    final_response.AddMember("error_code", error_code, allocator);

    rapidjson::Value error_msg_value;
    if (error_msg.empty()) {
        error_msg_value.SetString(ErrorMessage.at(error_code).c_str(),
                ErrorMessage.at(error_code).length(), allocator);
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

} // namespace uskit
