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

#include "backend_controller.h"
#include "backend_service.h"
#include "utils.h"

namespace uskit {

BackendController::BackendController(
        const BackendService* service,
        expression::ExpressionContext& context) :
        _service(service),
        _context("backend_controller", context),
        _response(&_context.allocator()) {}

BackendController::~BackendController() {}

int BackendController::build_request() {
    // Build request with policy of associated backend service
    if (_cancel_order == std::string("ALL")) {
        _done = std::unique_ptr<google::protobuf::Closure>(new AllCancelRPC(this, _cntls_call_ids));
    } else if (_cancel_order == std::string("PRIORITY")) {
        _done = std::unique_ptr<google::protobuf::Closure>(
                new PriorityCancelRPC(this, _cntls_call_ids, _priority));
    } else if (_cancel_order == std::string("HIERACHY")) {
        _done = std::unique_ptr<google::protobuf::Closure>(
                new HierarchyCancelRPC(this, _cntls_call_ids, _priority));
    }
    if (_service->build_request(this) != 0) {
        return -1;
    }
    return 0;
}

int BackendController::build_request(
        const BackendEngine* backend_engine,
        const std::unordered_map<std::string, FlowConfig>* flow_map,
        const RankEngine* rank_engine) {
    _jump_done = std::unique_ptr<google::protobuf::Closure>(
            new JumpRPC(this, flow_map, backend_engine, rank_engine));
    if (_service->build_request(backend_engine, this, flow_map, rank_engine) != 0) {
        return -1;
    }
    return 0;
}

int BackendController::parse_response() {
    // Parse response with policy of associated backend service
    if (_service->parse_response(this) != 0) {
        return -1;
    }
    return 0;
}

int BackendController::set_call_ids(const std::vector<CallIdPriorityPair>& call_ids) {
    _cntls_call_ids = std::vector<CallIdPriorityPair>(call_ids.begin(), call_ids.end());
    return 0;
}

const std::vector<CallIdPriorityPair>& BackendController::get_call_ids() const {
    return _cntls_call_ids;
}

int BackendController::set_cancel_order(const std::string& cancel_order) {
    _cancel_order = cancel_order;
    return 0;
}

const std::string& BackendController::get_cancel_order() const {
    return _cancel_order;
}

const std::string& BackendController::service_name() const {
    return _service->name();
}

BRPC_NAMESPACE::Controller& BackendController::brpc_controller() {
    return _brpc_cntl;
}

BackendResponse& BackendController::response() {
    return _response;
}

expression::ExpressionContext& BackendController::context() {
    return _context;
}

BackendController* build_backend_controller(
        const BackendService* service,
        expression::ExpressionContext& context) {
    BackendController* cntl = nullptr;
    const BRPC_NAMESPACE::AdaptiveProtocolType protocol = service->protocol();
    bool is_dynamic = service->is_dynamic();

    // Currently supported protocols: HTTP, Redis.
    if (protocol == BRPC_NAMESPACE::PROTOCOL_HTTP && !is_dynamic) {
        cntl = new HttpController(service, context);
    } else if (protocol == BRPC_NAMESPACE::PROTOCOL_HTTP && is_dynamic) {
        cntl = new DynamicHTTPController(service, context);
    } else if (protocol == BRPC_NAMESPACE::PROTOCOL_REDIS) {
        cntl = new RedisController(service, context);
    }

    if (cntl == nullptr) {
        US_LOG(WARNING) << "Failed to create backend controller for service [" << service->name()
                        << "]";
    }

    return cntl;
}

}  // namespace uskit
