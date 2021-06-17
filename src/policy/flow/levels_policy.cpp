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

#include "policy/flow/levels_policy.h"
#include "expression/expression.h"

namespace uskit {
namespace policy {
namespace flow {

int CascadeAsyncPolicy::init_post_process(const FlowNodeConfig& node_config) {
    if (AsyncGlobalPolicy::init_post_process(node_config) != 0) {
        return -1;
    }
    std::unordered_map<std::string, FlowConfig> cascade_flow_map;
    for (auto iter = _flow_map.begin(); iter != _flow_map.end(); ++iter) {
        if (iter->second.deliver_config_size() > 0) {
            for (auto deliver_iter = iter->second.deliver_beg();
                 deliver_iter != iter->second.deliver_end();
                 ++deliver_iter) {
                std::string flow_name = deliver_iter->first;
                if (_flow_map.find(flow_name) != _flow_map.end()) {
                    US_LOG(ERROR) << "flow name: [" << flow_name
                                  << "] is reserved by system. User defined flow name should not "
                                     "be end with \"__NEXT\"";
                    return -1;
                }
                FlowConfig flow_config;
                flow_config.init(std::move(deliver_iter->second));
                cascade_flow_map.emplace(flow_name, std::move(flow_config));
            }
        }
        iter->second.deliver_clear();
    }
    typedef std::unordered_map<std::string, FlowConfig>::iterator FlowMapIter;
    _flow_map.insert(
            std::move_iterator<FlowMapIter>(cascade_flow_map.begin()),
            std::move_iterator<FlowMapIter>(cascade_flow_map.end()));
    cascade_flow_map.clear();
    return 0;
}

int CascadeAsyncPolicy::get_filterout_services(const rapidjson::Value& response,
        std::shared_ptr<policy::FlowPolicyHelper> helper) const {
    auto config_iter = _flow_map.find(helper->_curr_flow);
    if (config_iter == _flow_map.end()) {
        US_LOG(WARNING) << "curr flow [" << helper->_curr_flow << "] not in map";
        return -1;
    }
    helper->_filterout_services = config_iter->second.filterout_by_response(response);
    return 0;   
}


}  // namespace flow
}  // namespace policy
}  // namespace uskit
