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

#ifndef USKIT_POLICY_FLOW_GLOBAL_POLICY_H
#define USKIT_POLICY_FLOW_GLOBAL_POLICY_H

#include "policy/flow_policy.h"
#include "policy/flow/recurrent_policy.h"
#include "dynamic_config.h"

namespace uskit {
namespace policy {
namespace flow {

class AsyncGlobalPolicy : public AsyncInsideNodePolicy {
public:
    int init(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config) override;
    int helper_ptr_init(HelperPtr helper) const override;
    virtual int init_post_process(const FlowNodeConfig& node_config);
    virtual int build_default_gc_config(FlowNodeConfig::GlobalCancelConfig* gc_config);
};

}  // namespace flow
}  // namespace policy
}  // namespace uskit

#endif  // USKIT_POLICY_FLOW_GLOBAL_POLICY_H
