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
#include <brpc/controller.h>
#include <brpc/redis.h>

#include "common.h"
#include "expression/expression.h"

namespace uskit {

// Forward declaration
class BackendService;

// A backend controller represents a single RPC call to a specific backend service
// Backend controller is a wrapper for brpc::Controller
class BackendController {
public:
    BackendController(const BackendService *service,
                      expression::ExpressionContext& context);
    virtual ~BackendController();

    // Build request for RPC
    // Returns 0 on success, -1 otherwise.
    int build_request();
    // Parse response received from RPC
    // Returns 0 on success, -1 otherwise.
    int parse_response();

    // Obtain the associated backend service
    const std::string& service_name() const;
    // Obtain the brpc::Controller associated with this backend controller
    brpc::Controller& brpc_controller();
    // Obtain the parsed response
    BackendResponse& response();
    // Obtain the context associated with this backend controller
    expression::ExpressionContext& context();

private:
    // Backend service that this backend controller will iteract with
    const BackendService* _service;
    // Underlying brpc::Controller
    brpc::Controller _brpc_cntl;
    // Context for expression evaluation
    expression::ExpressionContext _context;

    // Parsed response
    BackendResponse _response;
};

// Controller for HTTP RPC
class HttpController : public BackendController {
public:
    HttpController(const BackendService* service, expression::ExpressionContext& context)
        : BackendController(service, context) {}
};

// Backend controller for Redis RPC
class RedisController : public BackendController {
public:
    RedisController(const BackendService* service, expression::ExpressionContext& context)
        : BackendController(service, context) {}
    brpc::RedisResponse& redis_response() { return _redis_response; }
private:
    // Redis response of brpc
    brpc::RedisResponse _redis_response;
};

// Backend controller factory
BackendController* build_backend_controller(const BackendService *service,
                                            expression::ExpressionContext& context);

} // namespace uskit

#endif // USKIT_BACKEND_CONTROLLER_H
