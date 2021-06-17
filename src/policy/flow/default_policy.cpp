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

#include "policy/flow/default_policy.h"
#include "expression/expression.h"

namespace uskit {
namespace policy {
namespace flow {

int DefaultPolicy::init(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config) {
    if (config_parser(config) != 0) {
        return -1;
    }
    if (flow_check(config) != 0) {
        return -1;
    }
    return 0;
}

int DefaultPolicy::init_intervene_by_json(USConfig& intervene_config_object) {
    if (!intervene_config_object.IsObject()) {
        US_LOG(ERROR) << "intervene config should be object, ["
                      << get_value_type(intervene_config_object) << "] given.";
        return -1;
    }
    for (auto& item : intervene_config_object.GetObject()) {
        std::string item_json_str = json_encode(item.value);
        InterveneFileConfig file_config;
        if (!json2pb::JsonToProtoMessage(item_json_str, &file_config)) {
            US_LOG(ERROR) << "Intervene file parse error: " << json_encode(item.value.GetObject());
            return -1;
        }
        _intervene_map.emplace(item.name.GetString(), file_config);
    }
    return 0;
}

int DefaultPolicy::config_parser(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config) {
    if (config.size() <= 0) {
        LOG(ERROR) << "Required at least one flow config";
        return -1;
    }

    for (int i = 0; i < config.size(); ++i) {
        const FlowNodeConfig& flow_node_config = config.Get(i);
        if (i == 0) {
            // Setup start flow node.
            _start_flow = flow_node_config.name();
        }
        FlowConfig flow_config(_curr_dir);
        if (flow_config.init(flow_node_config) != 0) {
            LOG(ERROR) << "Failed to parse flow config [" << flow_node_config.name() << "]";
            return -1;
        }
        if (flow_node_config.has_intervene_config()) {
            const InterveneConfig inter_config = flow_node_config.intervene_config();
            std::string file_name = inter_config.intervene_file();
            InterveneFileConfig intervene_file;
            if (_curr_dir == "") {
                auto iter = _intervene_map.find(file_name);
                if (iter == _intervene_map.end()) {
                    US_LOG(ERROR) << "Intervene file [" << file_name << "] not found in request";
                    return -1;
                } else {
                    intervene_file = iter->second;
                }
            } else {
                std::string intervene_config_file(_curr_dir + file_name);
                if (ReadProtoFromTextFile(intervene_config_file, &intervene_file) != 0) {
                    US_LOG(ERROR) << "Failed to parse file: " << _curr_dir
                                  << file_name;
                    return -1;
                };
            }
            if (flow_config.set_intervene_config(inter_config, intervene_file) != 0) {
                US_LOG(ERROR) << "Failed to initialize intervene by file: ["
                              << file_name << "]";
                return -1;
            }
        }
        _flow_map.emplace(flow_node_config.name(), std::move(flow_config));
    }
    return 0;
}

int DefaultPolicy::flow_check(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config) {
    // Make sure all next flow nodes exist.
    for (const auto& flow_node_config : config) {
        if (flow_node_config.has_next() &&
            _flow_map.find(flow_node_config.next()) == _flow_map.end()) {
            LOG(ERROR) << "Flow node [" << flow_node_config.next() << "] not found";
            return -1;
        }
    }
    return 0;
}

int DefaultPolicy::kernel_process(
        expression::ExpressionContext& flow_context,
        const FlowConfig& flow_config,
        HelperPtr helper) const {
    std::string curr_flow = helper->_curr_flow;
    // General routin in one flow node:
    // 1. def; 2. recall; 3. merge recall results; 4. rank; 5. output

    if (flow_config.recall_run_def(flow_context) != 0) {
        US_LOG(ERROR) << "Failed in define before recall";
        return -1;
    }
    if (flow_config.recall(this, flow_context) != 0) {
        US_LOG(ERROR) << "Failed to recall for flow [" << curr_flow << "]";
        return -1;
    }
    US_DLOG(INFO) << "after recall: " << flow_context.str();
    if (flow_config.rank(this, flow_context) != 0) {
        return -1;
    }
    if (flow_config.output(flow_context) != 0) {
        return -1;
    }
    return 0;
}

int DefaultPolicy::single_node(
        expression::ExpressionContext& flow_context,
        expression::ExpressionContext& top_context,
        HelperPtr helper) const {
    std::string curr_flow = helper->_curr_flow;
    std::string intervene_flow = "";
    while (true) {
        const FlowConfig& flow_config = _flow_map.at(curr_flow);
        if (flow_config.get_intervene_flow(flow_context, intervene_flow) != 0) {
            US_LOG(ERROR) << "flow get intervene flow failed: [" << curr_flow << "]";
            return -1;
        }
        US_DLOG(INFO) << "intervene flow: [" << intervene_flow << "]";
        if (intervene_flow == "" || _flow_map.find(intervene_flow) == _flow_map.end()) {
            US_DLOG(INFO) << "intervene flow missed or empty";
            break;
        } else {
            curr_flow = intervene_flow;
            helper->_curr_flow = curr_flow;
        }
    }
    const FlowConfig& flow_config = _flow_map.at(curr_flow);

    if (kernel_process(flow_context, flow_config, helper) != 0) {
        return -1;
    }
    rapidjson::Value* flow_output = flow_context.get_variable("output");
    rapidjson::Value* result = top_context.get_variable("result");
    if (flow_output != nullptr && flow_output->IsObject()) {
        if (merge_json_objects(*result, *flow_output, top_context.allocator()) != 0) {
            US_LOG(ERROR) << "flow output merge error";
            return -1;
        }
    }
    return 0;
}

int DefaultPolicy::run(USRequest& request, USResponse& response) const {
    if (inner_run(request, response, std::make_shared<FlowPolicyHelper>()) != 0) {
        return -1;
    }
    return 0;
}

int DefaultPolicy::inner_run(USRequest& request, USResponse& response, HelperPtr helper) const {
    // context saved shared variables from input i.e. "request" and variables
    // to outout i.e. "backend" & "result"
    expression::ExpressionContext top_context("top context", response.GetAllocator());
    top_context.set_variable("request", request);
    top_context.set_variable("backend", rapidjson::Value().SetObject());
    top_context.set_variable("result", rapidjson::Value().SetObject());

    std::string curr_flow = _start_flow;
    US_DLOG(INFO) << "Start from flow node: " << curr_flow;
    // go through from 1st flow to next ... end
    // UNLESS:
    // 1. flow node not found
    // 2. error in def OR recall OR rank OR output
    if (helper_ptr_init(helper) != 0) {
        US_LOG(ERROR) << "call ids vector pointer init error";
        return -1;
    }
    while (true) {
        US_DLOG(INFO) << "Running flow node [" << curr_flow << "]";
        if (_flow_map.find(curr_flow) == _flow_map.end()) {
            US_LOG(ERROR) << "Flow node [" << curr_flow << "] not found";
            return -1;
        }
        helper->_curr_flow = curr_flow;
        expression::ExpressionContext flow_context("flow block", top_context);
        if (single_node(flow_context, top_context, helper) != 0) {
            US_LOG(ERROR) << "Flow node [" << curr_flow << "] running error";
            return -1;
        }
        rapidjson::Value* flow_next = flow_context.get_variable("next");
        if (flow_next != nullptr) {
            // Get next flow node.
            curr_flow = flow_next->GetString();
        } else {
            break;
        }
    }
    rapidjson::Value* result = top_context.get_variable("result");
    result->Swap(response);
    return 0;
}

int DefaultPolicy::set_backend_engine(std::shared_ptr<BackendEngine> backend_engine) {
    _backend_engine = backend_engine;
    return 0;
}

std::shared_ptr<BackendEngine> DefaultPolicy::get_backend_engine() {
    return _backend_engine;
}

int DefaultPolicy::set_rank_engine(std::shared_ptr<RankEngine> rank_engine) {
    _rank_engine = rank_engine;
    return 0;
}
int DefaultPolicy::backend_run(
        const std::vector<std::pair<std::string, int>>& recall_services,
        expression::ExpressionContext& context,
        const std::string& cancel_order) const {
    return _backend_engine->run(recall_services, context, cancel_order);
}
int DefaultPolicy::backend_run(
        const FlowRecallConfig* recall_config,
        std::vector<std::shared_ptr<uskit::expression::ExpressionContext>>& context_vec,
        std::shared_ptr<policy::FlowPolicyHelper> helper) const {
    return _backend_engine->run(this, recall_config, context_vec, helper);
}

int DefaultPolicy::rank_run(
        const std::string& name,
        RankCandidate& rank_candidate,
        RankResult& rank_result,
        expression::ExpressionContext& context) const {
    return _rank_engine->run(name, rank_candidate, rank_result, context);
}

size_t DefaultPolicy::get_service_index(const std::string& service_name) const {
    return _backend_engine->get_service_index(service_name);
}
bool DefaultPolicy::has_service(const std::string& service_name) const {
    return _backend_engine->has_service(service_name);
}

std::unordered_map<std::string, FlowConfig>::const_iterator DefaultPolicy::get_flow_config(
        const std::string& flow_name) const {
    return _flow_map.find(flow_name);
}

std::unordered_map<std::string, FlowConfig>::const_iterator DefaultPolicy::flow_map_end() const {
    return _flow_map.end();
}

int DefaultPolicy::get_filterout_services(
        const rapidjson::Value& response,
        std::shared_ptr<policy::FlowPolicyHelper> helper) const {
    US_DLOG(INFO) << "Default Policy with response type: " << get_value_type(response)
                  << " and helper vec size: " << helper->_filterout_services.size();
    return 0;
}

}  // namespace flow
}  // namespace policy
}  // namespace uskit
