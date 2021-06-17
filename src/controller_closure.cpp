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

#include "utils.h"
#include "brpc.h"
#include "policy/flow_policy.h"
#include "controller_closure.h"
#include "backend_controller.h"

namespace uskit {

BaseRPCClosure::BaseRPCClosure(
        BackendController* cntl,
        const std::string& name,
        const policy::FlowPolicy* flow_policy) :
        _closure_name(name),
        _rpc_ids(cntl->get_call_ids()),
        _cntl(cntl),
        _self_priority(cntl->get_priority()),
        _flow_policy(flow_policy) {
    US_DLOG(INFO) << _closure_name << " Closure constructed.";
}

std::unique_ptr<google::protobuf::Closure> build_controller_closure(
        const std::string& cancel_order,
        BackendController* cntl,
        const policy::FlowPolicy* flow_policy) {
    if (flow_policy) {
        return std::make_unique<GlobalClosure>(cntl, "GLOBAL_CANCEL", flow_policy);
    } else if (cancel_order == "ALL") {
        return std::make_unique<AllCancelClosure>(cntl, cancel_order);
    } else if (cancel_order == "PRIORITY") {
        return std::make_unique<PriorityClosure>(cntl, cancel_order);
    } else if (cancel_order == "HIERACHY") {
        return std::make_unique<HierarchyClosure>(cntl, cancel_order);
    } else {
        return std::make_unique<DoNothingClosrue>();
    }
}

void BaseRPCClosure::Run() {
    US_DLOG(INFO) << "all cancel rpc run";
    if (_cntl->brpc_controller().Failed()) {
        US_LOG(INFO) << "The service: " << _cntl->service_name().c_str()
                     << " failed, live others alive.";
        return;
    }
    for (auto rpc_id : _rpc_ids) {
        US_DLOG(INFO) << "start cancel rpc_id: " << rpc_id.first.value;
        cancel(rpc_id);
        US_DLOG(INFO) << "end cancel rpc_id: " << rpc_id.first.value;
    }
}

void AllCancelClosure::cancel(CallIdPriorityPair rpc_id) const {
    BRPC_NAMESPACE::StartCancel(rpc_id.first);
}

void PriorityClosure::cancel(CallIdPriorityPair rpc_id) const {
    if (rpc_id.second <= _self_priority) {
        BRPC_NAMESPACE::StartCancel(rpc_id.first);
        US_DLOG(INFO) << "cancel rpc id: " << rpc_id.first.value;
    }
}

void HierarchyClosure::cancel(CallIdPriorityPair rpc_id) const {
    if (rpc_id.second == _self_priority) {
        BRPC_NAMESPACE::StartCancel(rpc_id.first);
        US_DLOG(INFO) << "cancel rpc id: " << rpc_id.first.value;
    }
}

void GlobalClosure::cancel(CallIdPriorityPair rpc_id) const {
    return;
}

void GlobalClosure::cancel() {
    if (_cntl->get_cancel_order() == std::string("GLOBAL_CANCEL")) {
        if (GlobalCancel() != 0) {
            US_LOG(WARNING) << "GLOBAL_CANCEL Failed";
            return;
        }
    } else {
        if (!ServiceFlagCheck()) {
            return;
        }
        US_DLOG(INFO) << "Service Flag Check Pass!!!";
        for (auto rpc_id : _rpc_ids) {
            US_DLOG(INFO) << "rpc id: " << rpc_id.first.value
                          << ", rpc priority: " << rpc_id.second;
            if (_cntl->get_cancel_order() == std::string("ALL") ||
                (_cntl->get_cancel_order() == std::string("PRIORITY") &&
                 rpc_id.second <= _cntl->get_priority()) ||
                (_cntl->get_cancel_order() == std::string("HIERACHY") &&
                 rpc_id.second == _cntl->get_priority())) {
                US_DLOG(INFO) << "start cancel rpc_id: " << rpc_id.first.value;
                BRPC_NAMESPACE::StartCancel(rpc_id.first);
                US_DLOG(INFO) << "cancel rpc id: " << rpc_id.first.value;
            }
        }
    }

    return;
}

bool GlobalClosure::ServiceFlagCheck() {
    std::lock_guard<std::mutex> lock(_cntl->context().parent()->_outer_mutex);
    bool success_flag = false;
    if (_cntl->run_service_suc_flag(success_flag) != 0) {
        US_LOG(WARNING) << "Failed to evaluate service [" << _cntl->service_name()
                        << "] success flag";
        return false;
    }
    return success_flag;
}

int GlobalClosure::ResponseCheck() {
    std::vector<std::string> parse_response_result;
    BRPC_NAMESPACE::Controller& brpc_cntl = _cntl->brpc_controller();
    Timer tm("parse_response_t_ms(" + _cntl->service_name() + ")");
    tm.start();

    if (brpc_cntl.Failed()) {
        US_LOG(WARNING) << "Failed to receive response from service [" << _cntl->service_name()
                        << "]"
                        << " remote_server=" << brpc_cntl.remote_side()
                        << " latency=" << brpc_cntl.latency_us() << "us"
                        << " error_msg=" << brpc_cntl.ErrorText();
        // Skip parsing
        return -1;
    }
    US_LOG(INFO) << "Received response from service [" << _cntl->service_name() << "]"
                 << " remote_server=" << brpc_cntl.remote_side()
                 << " latency=" << brpc_cntl.latency_us() << "us";
    // Parse response
    std::lock_guard<std::mutex> lock(_cntl->context().parent()->_outer_mutex);
    expression::ExpressionContext* context = &_cntl->context();  // node name: flow block"

    if (_cntl->parse_response() == 0) {
        rapidjson::Value* backend_result = context->get_variable("backend");
        rapidjson::Value service_name;
        service_name.SetString(
                _cntl->service_name().c_str(),
                _cntl->service_name().length(),
                context->allocator());
        US_DLOG(INFO) << "service [" << _cntl->service_name()
                      << "] response: " << json_encode(_cntl->response());
        backend_result->AddMember(service_name, _cntl->response(), context->allocator());
        US_DLOG(INFO) << "backend_result source: " << json_encode(*backend_result);
        US_DLOG(INFO) << "backend_result: "
                      << json_encode(*(context->get_variable("backend"))).c_str();
        service_name.SetString(
                _cntl->service_name().c_str(),
                _cntl->service_name().length(),
                context->allocator());
        parse_response_result.push_back(_cntl->service_name());
    } else {
        US_LOG(ERROR) << "Service [" << _cntl->service_name() << "]"
                      << " response parsed failed";
        return -1;
    }
    tm.stop();
    return 0;
}

int GlobalClosure::GlobalCancel() {
    US_DLOG(INFO) << "[" << _cntl->_flow_name << "] Running GlobalCancel";
    if (_cntl->_call_ids_ptr == nullptr) {
        US_LOG(WARNING) << "cancel order: GLOBAL_CANCEL not supported other than "
                           "global flow policy";
        return -1;
    }
    // quit config check:
    auto curr_node_config = _flow_policy->get_flow_config(_cntl->_flow_name);
    // construct dummy context combine result from context_array
    USResponse tmp_response = USResponse(rapidjson::kObjectType);
    expression::ExpressionContext dummy_context("dummy_top_context", tmp_response.GetAllocator());
    rapidjson::Value* request_val = _cntl->context().get_variable("request");
    rapidjson::Value copyvalue(*request_val, dummy_context.allocator());
    dummy_context.set_variable("request", copyvalue);
    dummy_context.set_variable("backend", rapidjson::Value().SetObject());
    for (size_t index = 0; index != _cntl->_service_context_index.size(); ++index) {
        expression::ExpressionContext* tmp_context = (*_cntl->_flow_context_array)[index].get();
        std::lock_guard<std::mutex> lock(tmp_context->_outer_mutex);
        US_DLOG(INFO) << "[GC] tmp_context->_outer_mutex: " << &(tmp_context->_outer_mutex);
        rapidjson::Value backcopyvalue(
                *(tmp_context->get_variable("backend")), dummy_context.allocator());
        if (!backcopyvalue.IsNull()) {
            dummy_context.merge_variable("backend", backcopyvalue);
            // US_DLOG(INFO) << "dummy_context: " << dummy_context.str();
        }
        // US_DLOG(INFO) << "index: (" << index << ") backend res: " << json_encode(backcopyvalue);
    }
    US_DLOG(INFO) << "[" << _cntl->_flow_name << "] GC backend merge complete";
    if (curr_node_config->second.run_quit_config(dummy_context)) {
        if (_cntl->_call_ids_ptr->quit_flow() != 0) {
            US_LOG(ERROR) << "quit flow ERROR";
            return -1;
        }
        US_DLOG(INFO) << "Quit flow success";
    } else {
        US_DLOG(INFO) << "Not ready to quit flow when service [" << _cntl->service_name()
                      << "] callback";
    }
    return 0;
}

int GlobalClosure::HelperInit(std::shared_ptr<policy::FlowPolicyHelper> helper) {
    helper->_call_ids_ptr = _cntl->_call_ids_ptr;
    helper->_curr_flow = _cntl->get_recall_next();
    return 0;
}

void GlobalClosure::Run() {
    if (DynamicHTTPController* dhc = dynamic_cast<DynamicHTTPController*>(_cntl)) {
        std::lock_guard<std::mutex> dyn_lock(dhc->_outer_mutex);
        for (auto brpc_iter = dhc->brpc_controller_list().begin();
             brpc_iter != dhc->brpc_controller_list().end();
             ++brpc_iter) {
            if (!brpc_iter->get()->Failed() &&
                brpc_iter->get()->response_attachment().size() == 0) {
                US_DLOG(INFO) << "not all dynamic services finished";
                return;
            } else {
                US_DLOG(INFO) << "dynamic services finished, Failed? "
                              << brpc_iter->get()->Failed();
            }
        }
    } else {
        US_DLOG(INFO) << "_cntl is not dynamichttpcontroller";
    }
    if (ResponseCheck() != 0) {
        US_LOG(ERROR) << "Response parse failed.";
        return;
    }
    std::lock_guard<std::mutex> lock(_cntl->context()._outer_mutex);
    // TODO:
    // Response check
    while (true) {
        std::string next_flow = _cntl->get_recall_next();
        if (next_flow.empty()) {
            US_DLOG(INFO) << _cntl->service_name() << " has no next";
            cancel();
            return;
        }
        US_DLOG(INFO) << "[" << _cntl->service_name() << "]"
                      << "Running flow node: [" << next_flow << "]";
        if (_flow_policy->get_flow_config(next_flow) == _flow_policy->flow_map_end()) {
            US_LOG(ERROR) << "Flow node [" << next_flow << "] not found";
            return;
        }

        std::string intervene_flow = "";
        auto flow_config_iter = _flow_policy->get_flow_config(next_flow);
        expression::ExpressionContext* top_context = &_cntl->context();
        // if intervene match, jump to target flow until no-matched
        while (true) {
            if (flow_config_iter->second.get_intervene_flow(*top_context, intervene_flow) != 0) {
                US_LOG(ERROR) << "flow get intervene flow failed: [" << next_flow << "]";
                return;
            }
            if (intervene_flow == "" ||
                _flow_policy->get_flow_config(intervene_flow) == _flow_policy->flow_map_end()) {
                break;
            } else {
                next_flow = intervene_flow;
            }
        }

        US_DLOG(INFO) << "top_context: " << top_context->str();
        // Local context
        std::vector<std::string> service_list = flow_config_iter->second.get_recall_service_list();
        for (auto service_name : service_list) {
            size_t index = _cntl->_service_context_index.find(service_name)->second;
            expression::ExpressionContext* tmp_context =
                    _cntl->_flow_context_array->at(index).get();
            std::lock_guard<std::mutex> serv_lock(tmp_context->_outer_mutex);
            rapidjson::Value* backend_val = top_context->get_variable("backend");
            US_DLOG(INFO) << "top_context:"
                          << json_encode(*(top_context->get_variable("backend"))).c_str();
            rapidjson::Value copyvalue(*backend_val, tmp_context->allocator());
            US_DLOG(INFO) << "copyvalue: " << json_encode(copyvalue);
            tmp_context->erase_variable("backend");
            tmp_context->set_variable("backend", copyvalue);
            US_DLOG(INFO) << "[" << service_name << "] backend in loop [" << index
                          << "]: " << json_encode(*(tmp_context->get_variable("backend"))).c_str();
        }

        // only effect under levers flow policy
        expression::ExpressionContext flow_context(
                "flow block " + _cntl->service_name(), *top_context);

        rapidjson::Value* backend_val = top_context->get_variable("backend");
        rapidjson::Value item(
                (*backend_val)[_cntl->service_name().c_str()], _cntl->context().allocator());
        policy::HelperPtr helper = std::make_shared<policy::FlowPolicyHelper>();
        HelperInit(helper);
        if (_flow_policy->get_filterout_services(item, helper) != 0) {
            US_LOG(ERROR) << "Failed to get filterouts";
            return;
        }

        if (flow_config_iter->second.recall(_flow_policy, *(_cntl->_flow_context_array), helper) !=
            0) {
            US_LOG(ERROR) << "Failed to recall to flow [" << next_flow << "]";
            return;
        }

        for (auto service_name : service_list) {
            size_t index = _cntl->_service_context_index.find(service_name)
                                   ->second;  // core dump if service not define
            expression::ExpressionContext* tmp_context =
                    _cntl->_flow_context_array->at(index).get();
            std::lock_guard<std::mutex> last_lock(top_context->parent()->_outer_mutex);
            US_DLOG(INFO) << "[Run] tmp_context->_outer_mutex: "
                          << &(top_context->parent()->_outer_mutex);
            US_DLOG(INFO) << "[" << next_flow << "][" << service_name << "] merging backend";
            rapidjson::Value* backend_val = tmp_context->get_variable("backend");
            rapidjson::Value backcopyvalue(*backend_val, top_context->allocator());
            if (!backend_val->IsNull()) {
                US_DLOG(INFO) << "top_context in last merging:"
                              << json_encode(*(top_context->get_variable("backend"))).c_str();
                top_context->merge_variable("backend", backcopyvalue);
            }
        }

        US_DLOG(INFO) << "after recall: " << flow_context.str();

        rapidjson::Value* flow_next = flow_context.get_variable("next");
        if (flow_next != nullptr) {
            // Get next flow node.
            next_flow = flow_next->GetString();
        } else {
            break;
        }
    }
    cancel();
    return;
}

}  // namespace uskit
