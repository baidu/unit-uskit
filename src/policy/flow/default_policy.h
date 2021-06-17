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

#ifndef USKIT_POLICY_FLOW_DEFAULT_POLICY_H
#define USKIT_POLICY_FLOW_DEFAULT_POLICY_H

#include "policy/flow_policy.h"
#include "dynamic_config.h"

namespace uskit {
namespace policy {
namespace flow {

// Default flow policy.
class DefaultPolicy : public FlowPolicy {
public:
    virtual int init(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config);
    int inner_run(USRequest& request, USResponse& response, HelperPtr helper) const;
    int init_intervene_by_json(USConfig& intervene_config_object) override;
    virtual int kernel_process(
            expression::ExpressionContext& flow_context,
            const FlowConfig& flow_config,
            HelperPtr helper) const;
    int single_node(
            expression::ExpressionContext& flow_context,
            expression::ExpressionContext& top_context,
            HelperPtr helper) const;
    virtual int config_parser(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config);
    int flow_check(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config);
    virtual int helper_ptr_init(HelperPtr helper) const {
        return 0;
    }
    int run(USRequest& request, USResponse& response) const override;
    int set_backend_engine(std::shared_ptr<BackendEngine> backend_engine) override;
    std::shared_ptr<BackendEngine> get_backend_engine();
    int set_rank_engine(std::shared_ptr<RankEngine> rank_engine) override;
    int backend_run(
            const std::vector<std::pair<std::string, int>>& recall_services,
            expression::ExpressionContext& context,
            const std::string& cancel_order) const override;
    int backend_run(
            const FlowRecallConfig* recall_config,
            std::vector<std::shared_ptr<uskit::expression::ExpressionContext>>& context_vec,
            std::shared_ptr<policy::FlowPolicyHelper> helper) const override;
    int rank_run(
            const std::string& name,
            RankCandidate& rank_candidate,
            RankResult& rank_result,
            expression::ExpressionContext& context) const override;
    size_t get_service_index(const std::string& service_name) const override;
    bool has_service(const std::string& service_name) const override;
    std::unordered_map<std::string, FlowConfig>::const_iterator get_flow_config(
            const std::string& flow_name) const override;
    std::unordered_map<std::string, FlowConfig>::const_iterator flow_map_end() const override;
    int get_filterout_services(
            const rapidjson::Value& response,
            std::shared_ptr<policy::FlowPolicyHelper> helper) const override;
protected:
    std::unordered_map<std::string, FlowConfig> _flow_map;
    std::string _start_flow;
    std::unordered_map<std::string, InterveneFileConfig> _intervene_map;
};

}  // namespace flow
}  // namespace policy
}  // namespace uskit

#endif  // USKIT_POLICY_FLOW_DEFAULT_POLICY_H
