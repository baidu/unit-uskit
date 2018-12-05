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

BackendController::BackendController(const BackendService* service,
                                     expression::ExpressionContext& context)
    : _service(service), _context("backend_controller", context),
      _response(&_context.allocator()) {
}

BackendController::~BackendController() {
}

int BackendController::build_request() {
    // Build request with policy of associated backend service
    if (_service->build_request(this) != 0) {
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

const std::string& BackendController::service_name() const {
    return _service->name();
}

brpc::Controller& BackendController::brpc_controller() {
    return _brpc_cntl;
}

BackendResponse& BackendController::response() {
    return _response;
}

expression::ExpressionContext& BackendController::context() {
    return _context;
}

BackendController* build_backend_controller(const BackendService* service,
                                            expression::ExpressionContext& context) {
    BackendController* cntl = nullptr;
    const brpc::AdaptiveProtocolType protocol = service->protocol();

    // Currently supported protocols: HTTP, Redis.
    if (protocol == brpc::PROTOCOL_HTTP) {
        cntl = new HttpController(service, context);
    } else if (protocol == brpc::PROTOCOL_REDIS) {
        cntl = new RedisController(service, context);
    }

    if (cntl == nullptr) {
        US_LOG(WARNING) << "Failed to create backend controller for service ["
            << service->name() << "]";
    }

    return cntl;
}

} // namespace uskit
