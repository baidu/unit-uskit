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

#ifndef USKIT_BACKEND_CONTROLLER_H
#define USKIT_BACKEND_CONTROLLER_H

#include <string>
#include "brpc.h"

#include "common.h"
#include "expression/expression.h"
#include "dynamic_config.h"
#include "rank_engine.h"
#include "utils.h"

namespace uskit {

// Forward declaration
class BackendService;
class BackendEngine;

// A backend controller represents a single RPC call to a specific backend service
// Backend controller is a wrapper for brpc::Controller
class BackendController {
public:
    BackendController(const BackendService* service, expression::ExpressionContext& context);
    virtual ~BackendController();

    // Build request for RPC
    // Returns 0 on success, -1 otherwise.
    int build_request();
    int build_request(
            const BackendEngine* backend_engine,
            const std::unordered_map<std::string, FlowConfig>* flow_map,
            const RankEngine* rank_engine);
    // Parse response received from RPC
    // Returns 0 on success, -1 otherwise.
    int parse_response();

    // Obtain the associated backend service
    const std::string& service_name() const;
    // Obtain the BRPC_NAMESPACE::Controller associated with this backend controller
    BRPC_NAMESPACE::Controller& brpc_controller();
    // Obtain the parsed response
    BackendResponse& response();
    // Obtain the context associated with this backend controller
    expression::ExpressionContext& context();
    // Set call ids with priority
    int set_call_ids(const std::vector<CallIdPriorityPair>& cntls_call_ids);
    // Obtain the call ids with priority
    const std::vector<CallIdPriorityPair>& get_call_ids() const;
    // Set cancel order
    int set_cancel_order(const std::string& cancel_order);
    // Obtain cancel order
    const std::string& get_cancel_order() const;

    int set_priority(int priority) {
        _priority = priority;
        return 0;
    }
    const int get_priority() const {
        return _priority;
    }

    int set_recall_next(const std::string& recall_next) {
        _recall_next = recall_next;
        return 0;
    }

    const std::string get_recall_next() const {
        return _recall_next;
    }

    std::unique_ptr<google::protobuf::Closure> _done;
    std::unique_ptr<google::protobuf::Closure> _jump_done;
    std::vector<expression::ExpressionContext*>* _flow_context_array;
    std::unordered_map<std::string, size_t> _service_context_index;
    // global call ids ptr
    std::shared_ptr<CallIdsVecThreadSafe> _call_ids_ptr;
    // name of current flow
    std::string _flow_name;

private:
    // Backend service that this backend controller will iteract with
    const BackendService* _service;
    // Underlying BRPC_NAMESPACE::Controller
    BRPC_NAMESPACE::Controller _brpc_cntl;
    // Context for expression evaluation
    expression::ExpressionContext _context;
    // Call Ids with priority
    std::vector<CallIdPriorityPair> _cntls_call_ids;
    // Cancel order
    std::string _cancel_order;
    // cntl priority
    int _priority;
    std::string _recall_next;
    // Parsed response
    BackendResponse _response;
};

// Controller for HTTP RPC
class HttpController : public BackendController {
public:
    HttpController(const BackendService* service, expression::ExpressionContext& context) :
            BackendController(service, context) {}
};

// Backend controller for Redis RPC
class RedisController : public BackendController {
public:
    RedisController(const BackendService* service, expression::ExpressionContext& context) :
            BackendController(service, context) {}
    BRPC_NAMESPACE::RedisResponse& redis_response() {
        return _redis_response;
    }

private:
    // Redis response of brpc
    BRPC_NAMESPACE::RedisResponse _redis_response;
};

// Backend controller for Dynamic HTTP RPC
class DynamicHTTPController : public BackendController {
public:
    DynamicHTTPController(const BackendService* service, expression::ExpressionContext& context) :
            BackendController(service, context) {}
    std::vector<std::unique_ptr<BRPC_NAMESPACE::Controller>>& brpc_controller_list() {
        return _brpc_cntls_list;
    }
    std::mutex _outer_mutex;

private:
    // List of BRPC_CONTROLLORS
    std::vector<std::unique_ptr<BRPC_NAMESPACE::Controller>> _brpc_cntls_list;
};

// Backend controller factory
BackendController* build_backend_controller(
        const BackendService* service,
        expression::ExpressionContext& context);

// Special dones for canceling another RPC.
class AllCancelRPC : public google::protobuf::Closure {
public:
    explicit AllCancelRPC(BackendController* cntl, const std::vector<CallIdPriorityPair>& rpc_ids) :
            _rpc_ids(rpc_ids),
            _cntl(cntl) {
        US_DLOG(INFO) << "AllCancelRPC constructed.";
    }

    void Run() {
        US_DLOG(INFO) << "all cancel rpc run";
        if (_cntl->brpc_controller().Failed()) {
            US_LOG(INFO) << "The service: " << _cntl->service_name().c_str()
                         << " failed, live others alive.";
            return;
        }
        for (auto rpc_id : _rpc_ids) {
            US_DLOG(INFO) << "start cancel rpc_id: " << rpc_id.first.value;
            BRPC_NAMESPACE::StartCancel(rpc_id.first);
            US_DLOG(INFO) << "end cancel rpc_id: " << rpc_id.first.value;
        }
    }

private:
    std::vector<CallIdPriorityPair> _rpc_ids;
    BackendController* _cntl;
};

class PriorityCancelRPC : public google::protobuf::Closure {
public:
    explicit PriorityCancelRPC(
            BackendController* cntl,
            const std::vector<CallIdPriorityPair>& rpc_ids,
            int priority) :
            _rpc_ids(rpc_ids),
            _self_priority(priority),
            _cntl(cntl) {
        US_DLOG(INFO) << "PriorityCancelRPC constructed.";
    }

    void Run() {
        US_DLOG(INFO) << "pri cancel rpc run";
        if (_cntl->brpc_controller().Failed()) {
            US_LOG(INFO) << "The service: " << _cntl->service_name().c_str()
                         << " failed, live others alive.";
            return;
        }
        for (auto rpc_id : _rpc_ids) {
            US_DLOG(INFO) << "start cancel rpc_id: " << rpc_id.first.value;
            if (rpc_id.second <= _self_priority) {
                BRPC_NAMESPACE::StartCancel(rpc_id.first);
                US_DLOG(INFO) << "cancel rpc id: " << rpc_id.first.value;
            }
        }
    }

private:
    std::vector<CallIdPriorityPair> _rpc_ids;
    int _self_priority;
    BackendController* _cntl;
};

class HierarchyCancelRPC : public google::protobuf::Closure {
public:
    explicit HierarchyCancelRPC(
            BackendController* cntl,
            const std::vector<CallIdPriorityPair>& rpc_ids,
            int priority) :
            _rpc_ids(rpc_ids),
            _self_priority(priority),
            _cntl(cntl) {
        US_DLOG(INFO) << "HierarchyCancelRPC constructed.";
    }

    void Run() {
        US_DLOG(INFO) << "hierarchy cancel rpc run";
        if (_cntl->brpc_controller().Failed()) {
            US_LOG(INFO) << "The service: " << _cntl->service_name().c_str()
                         << " failed, live others alive.";
            return;
        }
        for (auto rpc_id : _rpc_ids) {
            US_DLOG(INFO) << "start cancel rpc_id: " << rpc_id.first.value;
            if (rpc_id.second == _self_priority) {
                BRPC_NAMESPACE::StartCancel(rpc_id.first);
                US_DLOG(INFO) << "cancel rpc id: " << rpc_id.first.value;
            }
        }
    }

private:
    std::vector<CallIdPriorityPair> _rpc_ids;
    int _self_priority;
    BackendController* _cntl;
};

// Special dones for canceling another RPC.
class JumpRPC : public google::protobuf::Closure {
public:
    explicit JumpRPC(
            BackendController* cntl,
            const std::unordered_map<std::string, FlowConfig>* flow_map,
            const BackendEngine* backend_engine,
            const RankEngine* rank_engine) :
            _cntl(cntl),
            _rpc_ids(cntl->get_call_ids()),
            _flow_map(flow_map),
            _backend(backend_engine),
            _rank(rank_engine) {}

    int ResponseCheck() {
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

    int GlobalCancel() {
        US_DLOG(INFO) << "[" << _cntl->_flow_name << "] Running GlobalCancel";
        if (_cntl->_call_ids_ptr == nullptr) {
            US_LOG(WARNING) << "cancel order: GLOBAL_CANCEL not supported other than "
                               "global flow policy";
            return -1;
        }
        // quit config check:
        const FlowConfig& curr_node_config = _flow_map->at(_cntl->_flow_name);
        // construct dummy context combine result from context_array
        USResponse tmp_response = USResponse(rapidjson::kObjectType);
        expression::ExpressionContext dummy_context("dummy_top_context", tmp_response.GetAllocator());
        rapidjson::Value* request_val = _cntl->context().get_variable("request");
        rapidjson::Value copyvalue(*request_val, dummy_context.allocator());
        dummy_context.set_variable("request", copyvalue);
        dummy_context.set_variable("backend", rapidjson::Value().SetObject());
        for (size_t index = 0; index != _cntl->_service_context_index.size(); ++index) {
            expression::ExpressionContext* tmp_context = (*_cntl->_flow_context_array)[index];
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
        if (curr_node_config.run_quit_config(dummy_context)) {
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
    void Run() {

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
                if (_cntl->get_cancel_order() == std::string("ALL")) {
                    for (auto rpc_id : _rpc_ids) {
                        BRPC_NAMESPACE::StartCancel(rpc_id.first);
                    }
                } else if (_cntl->get_cancel_order() == std::string("PRIORITY")) {
                    for (auto rpc_id : _rpc_ids) {
                        if (rpc_id.second <= _cntl->get_priority()) {
                            BRPC_NAMESPACE::StartCancel(rpc_id.first);
                        }
                    }
                } else if (_cntl->get_cancel_order() == std::string("HIERACHY")) {
                    for (auto rpc_id : _rpc_ids) {
                        US_DLOG(INFO) << "start cancel rpc_id: " << rpc_id.first.value;
                        if (rpc_id.second == _cntl->get_priority()) {
                            BRPC_NAMESPACE::StartCancel(rpc_id.first);
                            US_DLOG(INFO) << "cancel rpc id: " << rpc_id.first.value;
                        }
                    }
                } else if (_cntl->get_cancel_order() == std::string("GLOBAL_CANCEL")) {
                    if (GlobalCancel() != 0) {
                        US_LOG(WARNING) << "GLOBAL_CANCEL Failed";
                        return;
                    }
                } else if (_cntl->get_cancel_order() != std::string("NONE")) {
                    US_LOG(ERROR) << "Wrong cancel order [" << _cntl->get_cancel_order()
                                  << "] given, only ALL, NONE or PRIORITY allowed";
                    return;
                }
                return;
            }
            US_DLOG(INFO) << "[" << _cntl->service_name() << "]"
                          << "Running flow node: [" << next_flow << "]";
            if (_flow_map->find(next_flow) == _flow_map->end()) {
                US_LOG(ERROR) << "Flow node [" << next_flow << "] not found";
                return;
            }
            const FlowConfig& flow_config = _flow_map->at(next_flow);
            expression::ExpressionContext* top_context = &_cntl->context();
            US_DLOG(INFO) << "top_context: " << top_context->str();
            // Local context
            std::vector<std::string> service_list = flow_config.get_recall_service_list();
            for (auto service_name : service_list) {
                size_t index = _cntl->_service_context_index.find(service_name)->second;
                expression::ExpressionContext* tmp_context = _cntl->_flow_context_array->at(index);
                std::lock_guard<std::mutex> serv_lock(tmp_context->_outer_mutex);
                rapidjson::Value* backend_val = top_context->get_variable("backend");
                US_DLOG(INFO) << "top_context:"
                              << json_encode(*(top_context->get_variable("backend"))).c_str();
                rapidjson::Value copyvalue(*backend_val, tmp_context->allocator());
                US_DLOG(INFO) << "copyvalue: " << json_encode(copyvalue);
                tmp_context->erase_variable("backend");
                tmp_context->set_variable("backend", copyvalue);
                US_DLOG(INFO) << "[" << service_name << "] backend in loop [" << index << "]: "
                              << json_encode(*(tmp_context->get_variable("backend"))).c_str();
            }
            /** FOR DEBUG
            for (auto service_name : service_list) {
                size_t index = _cntl->_service_context_index.find(service_name)->second;
                expression::ExpressionContext* tmp_context = _cntl->_flow_context_array->at(index);
                std::string ifstat = tmp_context->has_variable("backend") ? "true" : "false";
                US_DLOG(INFO) << "If service [" << service_name << "] has backend: " << ifstat;
                if (tmp_context->has_variable("backend")) {
                    US_DLOG(INFO) << "[" << service_name << "]: " << tmp_context->str();
                }
            }
            **/
            expression::ExpressionContext flow_context(
                    "flow block " + _cntl->service_name(), *top_context);
            if (flow_config.recall(
                        _backend,
                        *(_cntl->_flow_context_array),
                        _flow_map,
                        _rank,
                        _cntl->_call_ids_ptr) != 0) {
                US_LOG(ERROR) << "Failed to recall to flow [" << next_flow << "]";
                return;
            }

            for (auto service_name : service_list) {
                size_t index = _cntl->_service_context_index.find(service_name)
                                       ->second;  // core dump if service not define
                expression::ExpressionContext* tmp_context = _cntl->_flow_context_array->at(index);
                std::lock_guard<std::mutex> last_lock(top_context->parent()->_outer_mutex);
                US_DLOG(INFO) << "[Run] tmp_context->_outer_mutex: " << &(top_context->parent()->_outer_mutex);
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

            /** not sure whether to rank and output or not
            if (flow_config.rank(_rank, flow_context) != 0) {
                return;
            }
            rapidjson::Value* flow_output = flow_context.get_variable("output");
            rapidjson::Value* result = top_context->get_variable("result");
            if (flow_output != nullptr && flow_output->IsObject()) {
                for (auto& m : flow_output->GetObject()) {
                    rapidjson::Value key(m.name, _cntl->response().GetAllocator());
                    result->AddMember(key, m.value, _cntl->response().GetAllocator());
                }
            }
            **/
            rapidjson::Value* flow_next = flow_context.get_variable("next");
            if (flow_next != nullptr) {
                // Get next flow node.
                next_flow = flow_next->GetString();
            } else {
                break;
            }
        }
        if (_cntl->get_cancel_order() == std::string("ALL")) {
            for (auto rpc_id : _rpc_ids) {
                BRPC_NAMESPACE::StartCancel(rpc_id.first);
            }
        } else if (_cntl->get_cancel_order() == std::string("PRIORITY")) {
            for (auto rpc_id : _rpc_ids) {
                if (rpc_id.second <= _cntl->get_priority()) {
                    BRPC_NAMESPACE::StartCancel(rpc_id.first);
                }
            }
        } else if (_cntl->get_cancel_order() == std::string("HIERACHY")) {
            for (auto rpc_id : _rpc_ids) {
                US_DLOG(INFO) << "start cancel rpc_id: " << rpc_id.first.value;
                if (rpc_id.second == _cntl->get_priority()) {
                    BRPC_NAMESPACE::StartCancel(rpc_id.first);
                    US_DLOG(INFO) << "cancel rpc id: " << rpc_id.first.value;
                }
            }
        } else if (_cntl->get_cancel_order() == std::string("GLOBAL_CANCEL")) {
            if (GlobalCancel() != 0) {
                US_LOG(WARNING) << "GLOBAL_CANCEL Failed";
                return;
            }
        } else if (_cntl->get_cancel_order() != std::string("NONE")) {
            US_LOG(ERROR) << "Wrong cancel order [" << _cntl->get_cancel_order()
                          << "] given, only ALL, NONE or PRIORITY allowed";
            return;
        }
    }

private:
    std::vector<CallIdPriorityPair> _rpc_ids;
    const std::unordered_map<std::string, FlowConfig>* _flow_map;
    BackendController* _cntl;
    const BackendEngine* _backend;
    const RankEngine* _rank;
};

}  // namespace uskit

#endif  // USKIT_BACKEND_CONTROLLER_H
