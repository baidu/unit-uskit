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

#include "policy/flow/global_policy.h"
#include "expression/expression.h"

namespace uskit {
namespace policy {
namespace flow {

int AsyncGlobalPolicy::init(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config) {
    if (DefaultPolicy::init(config) != 0) {
        return -1;
    }
    LOG(INFO) << "GLOBAL INIT";
    if (init_post_process(config.Get(0)) != 0) {
        LOG(ERROR) << "post process of initialize failed";
        return -1;
    }
    return 0;
}

int AsyncGlobalPolicy::init_post_process(const FlowNodeConfig& node_config) {
    // Go through _flow_map, set global config for each node from start flow
    FlowNodeConfig temp_node_config;
    if (!node_config.has_global_cancel_config()) {
        LOG(WARNING) << "The QUIT IF CONFIG has not been set in fisrt flow node, construct default "
                        "config";
        if (build_default_gc_config(temp_node_config.mutable_global_cancel_config()) != 0) {
            LOG(ERROR) << "Default quit config construct failed";
            return -1;
        }
    }
    for (auto iter = _flow_map.begin(); iter != _flow_map.end(); ++iter) {
        if (!temp_node_config.has_global_cancel_config()) {
            if (iter->second.set_quit_conifg(node_config.global_cancel_config()) != 0) {
                LOG(ERROR) << "Failed to parse flow config [" << iter->first
                           << "] with quit config";
                return -1;
            }
        } else if (iter->second.set_quit_conifg(temp_node_config.global_cancel_config()) != 0) {
            LOG(ERROR) << "Failed to parse flow config [" << iter->first << "] with quit config";
            return -1;
        }
    }
    return 0;
}

int AsyncGlobalPolicy::helper_ptr_init(HelperPtr helper) const {
    helper->_call_ids_ptr = std::make_shared<CallIdsVecThreadSafe>();
    return helper->_call_ids_ptr->init(_backend_engine->get_service_size());
}

int AsyncGlobalPolicy::build_default_gc_config(FlowNodeConfig::GlobalCancelConfig* gc_config) {
    gc_config->add_cond("false");
    return 0;
}

}  // namespace flow
}  // namespace policy
}  // namespace uskit
