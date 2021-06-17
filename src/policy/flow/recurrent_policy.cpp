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

int AsyncInsideNodePolicy::kernel_process(
        expression::ExpressionContext& flow_context,
        const FlowConfig& flow_config,
        HelperPtr helper) const {
    std::string curr_flow = helper->_curr_flow;
    // define local context, allocator should be DIFFERENT between services.
    size_t _service_size = _backend_engine->get_service_size();
    std::vector<std::shared_ptr<expression::ExpressionContext>> flow_context_array;
    std::vector<std::shared_ptr<USResponse>> toy_document_vector;
    for (size_t i = 0; i < _service_size; i++) {
        toy_document_vector.push_back(std::make_shared<USResponse>(rapidjson::kObjectType));
        flow_context_array.push_back(std::make_shared<expression::ExpressionContext>(
                "top_context", toy_document_vector[i]->GetAllocator()));

        rapidjson::Value* request_val = flow_context.get_variable("request");
        rapidjson::Value copyvalue(*request_val, flow_context_array[i]->allocator());
        flow_context_array[i]->set_variable("request", copyvalue);

        rapidjson::Value* backend_val = flow_context.get_variable("backend");
        if (backend_val) {
            rapidjson::Value back_copyvalue(*backend_val, flow_context_array[i]->allocator());
            flow_context_array[i]->set_variable("backend", back_copyvalue);
        } else {
            flow_context_array[i]->set_variable("backend", rapidjson::Value().SetObject());
        }

        rapidjson::Value* result_val = flow_context.get_variable("result");
        if (result_val) {
            rapidjson::Value result_copyvalue(*result_val, flow_context_array[i]->allocator());
            flow_context_array[i]->set_variable("result", result_copyvalue);
        } else {
            flow_context_array[i]->set_variable("result", rapidjson::Value().SetObject());
        }
    }

    // General routin in one flow node:
    // 1. def; 2. recall; 3. merge recall results; 4. rank; 5. output
    if (flow_config.recall_run_def(flow_context) != 0) {
        US_LOG(ERROR) << "Failed in define before recall";
        return -1;
    }

    if (flow_config.recall(this, flow_context_array, helper) != 0) {
        US_LOG(ERROR) << "Failed to recall for flow [" << curr_flow << "]";
        return -1;
    }
    rapidjson::Value success_recall_services(rapidjson::kArrayType);
    std::vector<std::string> curr_flow_recall_services;
    if (helper->_intervene_service != "") {
        curr_flow_recall_services.push_back(helper->_intervene_service);
    } else {
        for (auto service_name : flow_config.get_recall_service_list()) {
            curr_flow_recall_services.push_back(service_name);
        }
    }
    for (auto service_name : curr_flow_recall_services) {
        if (!_backend_engine->has_service(service_name)) {
            US_DLOG(WARNING) << "service [" << service_name << "] is unknown";
            continue;
        }
        size_t index = _backend_engine->get_service_index(service_name);
        auto tmp_context = flow_context_array[index];
        rapidjson::Value* backend_val = tmp_context->get_variable("backend");
        US_DLOG(INFO) << "service [" << service_name
                      << "] backend res: " << json_encode(*backend_val);
        if (!backend_val->IsNull()) {
            flow_context.merge_variable("backend", *backend_val);
            success_recall_services.PushBack(
                    rapidjson::Value(service_name.c_str(), flow_context.allocator()).Move(),
                    flow_context.allocator());
        }
    }
    flow_context.set_variable("recall", success_recall_services);
    US_DLOG(INFO) << "after recall: " << flow_context.str();
    if (flow_config.rank(this, flow_context) != 0) {
        US_LOG(ERROR) << "Failed to rank for flow [" << curr_flow << "]";
        return -1;
    }

    if (flow_config.output(flow_context) != 0) {
        US_LOG(ERROR) << "Failed to generate output for flow [" << curr_flow << "]";
        return -1;
    }
    return 0;
}

}  // namespace flow
}  // namespace policy
}  // namespace uskit
