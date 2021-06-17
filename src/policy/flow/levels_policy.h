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
#ifndef USKIT_POLICY_FLOW_LEVELS_POLICY_H
#define USKIT_POLICY_FLOW_LEVELS_POLICY_H

#include "policy/flow/global_policy.h"
#include "dynamic_config.h"

namespace uskit {
namespace policy {
namespace flow {

class CascadeAsyncPolicy : public AsyncGlobalPolicy {
    int init_post_process(const FlowNodeConfig& node_config) override;
    int get_filterout_services(const rapidjson::Value& response,
            std::shared_ptr<policy::FlowPolicyHelper> helper) const override;
};

}  // namespace flow
}  // namespace policy
}  // namespace uskit

#endif  // USKIT_POLICY_FLOW_LEVELS_POLICY_H
