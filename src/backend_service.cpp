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

#include <butil/logging.h>
#include "backend_controller.h"
#include "backend_service.h"
#include "policy/policy_manager.h"
#include "utils.h"

namespace uskit
{

BackendService::BackendService() {
}

BackendService::~BackendService() {
}

int BackendService::init(const ServiceConfig& service_config, Backend* backend) {
    _backend = backend;
    _name = service_config.name();
    if (service_config.has_request()) {
        const RequestConfig& request_config = service_config.request();
        std::string request_policy_name(service_config.request_policy());
        if (request_policy_name == "default") {
            const brpc::AdaptiveProtocolType protocol = _backend->channel()->options().protocol;
            if (protocol == brpc::PROTOCOL_HTTP) {
                request_policy_name = "http_default";
            } else if (protocol == brpc::PROTOCOL_REDIS) {
                request_policy_name = "redis_default";
            }
        }
        // Get backend request policy
        _request_policy = std::unique_ptr<policy::BackendRequestPolicy>(
            policy::PolicyManager::instance().get_request_policy(request_policy_name));

        if (!_request_policy) {
            LOG(ERROR) << "Request policy [" << request_policy_name << "] not found";
            return -1;
        }

        if (_request_policy->init(request_config, _backend) != 0) {
            LOG(ERROR) << "Failed to initialize request policy [" << request_policy_name << "]";
            return -1;
        }
    }

    if (service_config.has_response()) {
        const ResponseConfig& response_config = service_config.response();
        std::string response_policy_name(service_config.response_policy());
        if (response_policy_name == "default") {
            const brpc::AdaptiveProtocolType protocol = _backend->channel()->options().protocol;
            if (protocol == brpc::PROTOCOL_HTTP) {
                response_policy_name = "http_default";
            } else if (protocol == brpc::PROTOCOL_REDIS) {
                response_policy_name = "redis_default";
            }
        }
        // Get backend response policy
        _response_policy = std::unique_ptr<policy::BackendResponsePolicy>(
            policy::PolicyManager::instance().get_response_policy(response_policy_name));

        if (!_response_policy) {
            LOG(ERROR) << "Response policy [" << response_policy_name << "] not found";
            return -1;
        }

        if (_response_policy->init(response_config, _backend) != 0) {
            LOG(ERROR) << "Failed to initialize response policy [" << response_policy_name << "]";
            return -1;
        }
    }

    return 0;
}

int BackendService::build_request(BackendController* cntl) const {
    if (_request_policy && _request_policy->run(cntl) != 0) {
        US_LOG(WARNING) << "Failed to build request for service [" << _name << "]";
        return -1;
    }
    return 0;
}

int BackendService::parse_response(BackendController* cntl) const {
    if (_response_policy && _response_policy->run(cntl) != 0) {
        US_LOG(WARNING) << "Failed to parse response for service [" << _name << "]";
        return -1;
    }
    return 0;
}

const std::string& BackendService::name() const {
    return _name;
}

const brpc::AdaptiveProtocolType BackendService::protocol() const {
    return _backend->protocol();
}

} // namespace uskit
