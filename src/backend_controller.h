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
#include "controller_closure.h"

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
    int build_request(const policy::FlowPolicy* flow_policy = nullptr);
    // Parse response received from RPC
    // Returns 0 on success, -1 otherwise.
    int parse_response();
    virtual int join();
    virtual int64_t get_latency_us();
    virtual bool failed();

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
    int get_priority() const {
        return _priority;
    }

    int set_recall_next(const std::string& recall_next) {
        _recall_next = recall_next;
        return 0;
    }

    const std::string& get_recall_next() const {
        return _recall_next;
    }

    int run_service_suc_flag(bool& bool_value);

    std::unique_ptr<google::protobuf::Closure> _done;
    std::vector<std::shared_ptr<uskit::expression::ExpressionContext> >* _flow_context_array;
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
    int join() override;
    int64_t get_latency_us() override;
    bool failed() override;
    std::mutex _outer_mutex;

private:
    // List of BRPC_CONTROLLORS
    std::vector<std::unique_ptr<BRPC_NAMESPACE::Controller>> _brpc_cntls_list;
};

// Backend controller factory
BackendController* build_backend_controller(
        const BackendService* service,
        expression::ExpressionContext& context);

}  // namespace uskit

#endif  // USKIT_BACKEND_CONTROLLER_H
