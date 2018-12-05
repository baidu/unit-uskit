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

#ifndef USKIT_POLICY_POLICY_MANAGER_H
#define USKIT_POLICY_POLICY_MANAGER_H

#include <memory>
#include "policy/backend_policy.h"
#include "policy/flow_policy.h"
#include "policy/rank_policy.h"

namespace uskit {
namespace policy {

template<typename T>
BackendRequestPolicy* build_request_policy() { return new T; }
typedef BackendRequestPolicy* (*RequestPolicyFunctionPtr)();

template<typename T>
BackendResponsePolicy* build_response_policy() { return new T; }
typedef BackendResponsePolicy* (*ResponsePolicyFunctionPtr)();

template<typename T>
FlowPolicy* build_flow_policy() { return new T; }
typedef FlowPolicy* (*FlowPolicyFunctionPtr)();

template <typename T>
RankPolicy* build_rank_policy() { return new T; }
typedef RankPolicy* (*RankPolicyFunctionPtr)();

// Class for managing policies.
class PolicyManager {
public:
    PolicyManager(const PolicyManager&) = delete;
    PolicyManager& operator=(const PolicyManager&) = delete;

    // Singleton
    static PolicyManager& instance();

    // Add backend request policy with specified name.
    void add_request_policy(const std::string& policy_name,
                            RequestPolicyFunctionPtr func_ptr);
    // Add backend response policy with specified name.
    void add_response_policy(const std::string& policy_name,
                             ResponsePolicyFunctionPtr func_ptr);
    // Add flow policy with specified name.
    void add_flow_policy(const std::string& policy_name,
                         FlowPolicyFunctionPtr func_ptr);
    // Add rank policy with specified name.
    void add_rank_policy(const std::string& policy_name,
                         RankPolicyFunctionPtr func_ptr);

    // Get backend request policy with specified name.
    BackendRequestPolicy* get_request_policy(const std::string& policy_name);
    // Get backend response policy with specified name.
    BackendResponsePolicy* get_response_policy(const std::string& policy_name);
    // Get flow policy with specified name.
    FlowPolicy* get_flow_policy(const std::string& policy_name);
    // Get rank policy with specified name.
    RankPolicy* get_rank_policy(const std::string& policy_name);

private:
    PolicyManager();

    std::unordered_map<std::string, RequestPolicyFunctionPtr> _request_policy_map;
    std::unordered_map<std::string, ResponsePolicyFunctionPtr> _response_policy_map;
    std::unordered_map<std::string, FlowPolicyFunctionPtr> _flow_policy_map;
    std::unordered_map<std::string, RankPolicyFunctionPtr> _rank_policy_map;
};


} // namespace policy

// Macro for backend request policy registering.
#define REGISTER_REQUEST_POLICY(policy_name, class_name) \
    policy::PolicyManager::instance().add_request_policy(policy_name, policy::build_request_policy<class_name>)

// Macro for backend response policy registering.
#define REGISTER_RESPONSE_POLICY(policy_name, class_name) \
    policy::PolicyManager::instance().add_response_policy(policy_name, policy::build_response_policy<class_name>)

// Macro for flow policy registering.
#define REGISTER_FLOW_POLICY(policy_name, class_name) \
    policy::PolicyManager::instance().add_flow_policy(policy_name, policy::build_flow_policy<class_name>)

// Macro for rank policy registering.
#define REGISTER_RANK_POLICY(policy_name, class_name) \
    policy::PolicyManager::instance().add_rank_policy(policy_name, policy::build_rank_policy<class_name>)

} // namespace uskit

#endif // USKIT_POLICY_POLICY_MANAGER_H
