// Copyright 2018 Baidu, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// 	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef USKIT_POLICY_FLOW_POLICY_H
#define USKIT_POLICY_FLOW_POLICY_H

#include "config.pb.h"
#include "common.h"
#include "backend_engine.h"
#include "dynamic_config.h"

#include <json2pb/json_to_pb.h>

namespace uskit {
namespace policy {

// Helper calss for flow policy
// Save data w.r.t. single request and pass through FlowPolicy's member functions
class FlowPolicyHelper {
public:
    std::string _curr_flow;
    std::string _policy_name;
    std::string _intervene_service;
    std::vector<std::string> _filterout_services;
    std::shared_ptr<CallIdsVecThreadSafe> _call_ids_ptr;
    std::shared_ptr<std::unordered_set<std::string>> _target_service_set;
    inline FlowPolicyHelper() :
            _policy_name(""), _intervene_service(""), _call_ids_ptr(nullptr), _target_service_set(nullptr) {}
};

typedef std::shared_ptr<FlowPolicyHelper> HelperPtr;

// Base class for flow policy.
class FlowPolicy {
public:
    FlowPolicy() :
            _curr_dir(""),
            _backend_engine(std::make_shared<BackendEngine>()),
            _rank_engine(std::make_shared<RankEngine>()) {}
    virtual ~FlowPolicy() {}

    virtual int init(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config) {
        return 0;
    }
    virtual int init_intervene_by_json(USConfig& intervene_config_object) = 0;
    virtual int set_curr_dir(std::string curr_dir) {
        _curr_dir = curr_dir;
        return 0;
    }
    virtual int run(USRequest& request, USResponse& response) const = 0;
    virtual int set_backend_engine(std::shared_ptr<BackendEngine> backend_engine) = 0;
    virtual int set_rank_engine(std::shared_ptr<RankEngine> rank_engine) = 0;
    virtual int backend_run(
            const std::vector<std::pair<std::string, int>>& recall_services,
            expression::ExpressionContext& context,
            const std::string& cancel_order) const = 0;
    virtual int backend_run(
            const FlowRecallConfig* recall_config,
            std::vector<std::shared_ptr<uskit::expression::ExpressionContext>>& context_vec,
            std::shared_ptr<policy::FlowPolicyHelper> helper) const = 0;
    virtual int rank_run(
            const std::string& name,
            RankCandidate& rank_candidate,
            RankResult& rank_result,
            expression::ExpressionContext& context) const = 0;
    virtual size_t get_service_index(const std::string& service_name) const = 0;
    virtual bool has_service(const std::string& service_name) const = 0;
    virtual std::unordered_map<std::string, FlowConfig>::const_iterator get_flow_config(
            const std::string& flow_name) const = 0;
    virtual std::unordered_map<std::string, FlowConfig>::const_iterator flow_map_end() const = 0;
    // return 0 and do nothing in default
    virtual int get_filterout_services(
            const rapidjson::Value& response,
            std::shared_ptr<policy::FlowPolicyHelper> helper) const = 0;

protected:
    std::string _curr_dir;
    std::shared_ptr<BackendEngine> _backend_engine;
    std::shared_ptr<RankEngine> _rank_engine;
};

}  // namespace policy
}  // namespace uskit

#endif  // USKIT_POLICY_FLOW_POLICY_H
