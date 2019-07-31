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

#include "policy/flow/recurrent_policy.h"
#include "expression/expression.h"

namespace uskit {
namespace policy {
namespace flow {

int RecurrentPolicy::init(const google::protobuf::RepeatedPtrField<FlowNodeConfig>& config) {
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
        FlowConfig flow_config;
        if (flow_config.init(flow_node_config) != 0) {
            LOG(ERROR) << "Failed to parse flow config [" << flow_node_config.name() << "]";
            return -1;
        }
        _flow_map.emplace(flow_node_config.name(), std::move(flow_config));
    }
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

int RecurrentPolicy::run(
        USRequest& request,
        USResponse& response,
        const BackendEngine* backend_engine,
        const RankEngine* rank_engine) const {
    expression::ExpressionContext top_context("top context", response.GetAllocator());
    top_context.set_variable("request", request);
    top_context.set_variable("backend", rapidjson::Value().SetObject());
    top_context.set_variable("result", rapidjson::Value().SetObject());
    std::string curr_flow = _start_flow;
    US_DLOG(INFO) << "Start from flow node: " << curr_flow;
    while (true) {
        US_DLOG(INFO) << "Running flow node [" << curr_flow << "]";
        if (_flow_map.find(curr_flow) == _flow_map.end()) {
            US_LOG(ERROR) << "Flow node [" << curr_flow << "] not found";
            return -1;
        }
        const FlowConfig& flow_config = _flow_map.at(curr_flow);

        // Local context.
        size_t service_size = backend_engine->get_service_size();
        std::vector<expression::ExpressionContext*> flow_context_array;
        std::vector<USResponse*> toy_document_vector;
        for (size_t i = 0; i < service_size; i++) {
            toy_document_vector.push_back(new USResponse(rapidjson::kObjectType));
            flow_context_array.push_back(new expression::ExpressionContext(
                    "top_context", toy_document_vector[i]->GetAllocator()));
            rapidjson::Value* request_val = top_context.get_variable("request");
            rapidjson::Value copyvalue(*request_val, flow_context_array[i]->allocator());
            flow_context_array[i]->set_variable("request", copyvalue);
            flow_context_array[i]->set_variable("backend", rapidjson::Value().SetObject());
            flow_context_array[i]->set_variable("result", rapidjson::Value().SetObject());
        }
        expression::ExpressionContext flow_context("flow block", top_context);
        if (flow_config.recall_run_def(flow_context) != 0) {
            US_LOG(ERROR) << "Failed in define before recall";
            for (size_t i = 0; i < service_size; i++) {
                delete toy_document_vector[i];
                delete flow_context_array[i];
            }
            toy_document_vector.clear();
            flow_context_array.clear();
            return -1;
        }

        if (flow_config.recall(backend_engine, flow_context_array, &_flow_map, rank_engine) != 0) {
            US_LOG(ERROR) << "Failed to recall for flow [" << curr_flow << "]";
            for (size_t i = 0; i < service_size; i++) {
                delete toy_document_vector[i];
                delete flow_context_array[i];
            }
            toy_document_vector.clear();
            flow_context_array.clear();
            return -1;
        }
        std::vector<std::string> service_list = flow_config.get_recall_service_list();
        for (auto service_name : service_list) {
            size_t index = backend_engine->get_service_index(service_name);
            expression::ExpressionContext* tmp_context = flow_context_array[index];
            rapidjson::Value* backend_val = tmp_context->get_variable("backend");
            US_DLOG(INFO) << "service [" << service_name << "] backend res: " << json_encode(*backend_val);
            if (!backend_val->IsNull()) {
                flow_context.merge_variable("backend", *backend_val);
            }
        }
        US_DLOG(INFO) << "after recall: " << flow_context.str();

        if (flow_config.rank(rank_engine, flow_context) != 0) {
            for (size_t i = 0; i < service_size; i++) {
                delete toy_document_vector[i];
                delete flow_context_array[i];
            }
            toy_document_vector.clear();
            flow_context_array.clear();
            return -1;
        }
        if (flow_config.output(flow_context) != 0) {
            for (size_t i = 0; i < service_size; i++) {
                delete toy_document_vector[i];
                delete flow_context_array[i];
            }
            toy_document_vector.clear();
            flow_context_array.clear();
            return -1;
        }
        rapidjson::Value* flow_output = flow_context.get_variable("output");
        rapidjson::Value* result = top_context.get_variable("result");
        if (flow_output != nullptr && flow_output->IsObject()) {
            for (auto& m : flow_output->GetObject()) {
                rapidjson::Value key(m.name, response.GetAllocator());
                result->AddMember(key, m.value, response.GetAllocator());
            }
        }

        for (size_t i = 0; i < service_size; i++) {
            delete toy_document_vector[i];
            delete flow_context_array[i];
        }
        toy_document_vector.clear();
        flow_context_array.clear();

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

}  // namespace flow
}  // namespace policy
}  // namespace uskit
