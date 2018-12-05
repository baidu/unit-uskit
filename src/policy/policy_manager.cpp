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

#include "policy_manager.h"

namespace uskit {
namespace policy {

PolicyManager::PolicyManager() {}

PolicyManager& PolicyManager::instance() {
    static PolicyManager instance;
    return instance;
}

void PolicyManager::add_request_policy(const std::string& policy_name,
                                       RequestPolicyFunctionPtr func_ptr) {
    _request_policy_map.emplace(policy_name, func_ptr);
}

void PolicyManager::add_response_policy(const std::string& policy_name,
                                        ResponsePolicyFunctionPtr func_ptr) {
    _response_policy_map.emplace(policy_name, func_ptr);
}

void PolicyManager::add_flow_policy(const std::string& policy_name,
                                    FlowPolicyFunctionPtr func_ptr) {
    _flow_policy_map.emplace(policy_name, func_ptr);
}

void PolicyManager::add_rank_policy(const std::string& policy_name,
                                    RankPolicyFunctionPtr func_ptr) {
    _rank_policy_map.emplace(policy_name, func_ptr);
}

BackendRequestPolicy* PolicyManager::get_request_policy(const std::string& policy_name) {
    auto iter = _request_policy_map.find(policy_name);
    if (iter != _request_policy_map.end()) {
        return (iter->second)();
    } else {
        LOG(ERROR) << "Request policy [" << policy_name << "] not found";
        return nullptr;
    }
}

BackendResponsePolicy* PolicyManager::get_response_policy(const std::string& policy_name) {
    auto iter = _response_policy_map.find(policy_name);
    if (iter != _response_policy_map.end()) {
        return (iter->second)();
    } else {
        LOG(ERROR) << "Response policy [" << policy_name << "] not found";
        return nullptr;
    }
}

FlowPolicy* PolicyManager::get_flow_policy(const std::string& policy_name) {
    auto iter = _flow_policy_map.find(policy_name);
    if (iter != _flow_policy_map.end()) {
        return (iter->second)();
    } else {
        LOG(ERROR) << "Flow policy [" << policy_name << "] not found";
        return nullptr;
    }
}

RankPolicy* PolicyManager::get_rank_policy(const std::string& policy_name) {
    auto iter = _rank_policy_map.find(policy_name);
    if (iter != _rank_policy_map.end()) {
        return (iter->second)();
    } else {
        LOG(ERROR) << "Rank policy [" << policy_name << "] not found";
        return nullptr;
    }
}

} // namespace policy
} // namespace uskit
