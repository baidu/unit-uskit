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

#ifndef USKIT_BACKEND_SERVICE_H
#define USKIT_BACKEND_SERVICE_H

#include <string>
#include "dynamic_config.h"
#include "backend.h"
#include "policy/backend_policy.h"

namespace uskit
{

// Forward declaration
class BackendController;

// A backend service represents an RPC specification, which can be used by backend
// controller to issue RPC.
// A backend service instance is corresponding to a service section in the `backend.conf`.
class BackendService {
public:
    BackendService();
    BackendService(BackendService&&) = default;
    virtual ~BackendService();

    // Initialize backend service from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const ServiceConfig& service_config, Backend* backend);
    // Build request for RPC
    // Returns 0 on success, -1 otherwise.
    int build_request(BackendController* cntl) const;
    // Parse response received from RPC
    // Returns 0 on success, -1 otherwise.
    int parse_response(BackendController* cntl) const;

    const std::string& name() const;
    // Obtain the protocol associated with this service.
    // Currently supported protocols: HTTP, Redis.
    const brpc::AdaptiveProtocolType protocol() const;

private:
    // Name of this service
    std::string _name;
    // Backend that this service belongs to
    Backend* _backend;

    // Policy for building backend request
    std::unique_ptr<policy::BackendRequestPolicy> _request_policy;
    // Policy for parsing backend response
    std::unique_ptr<policy::BackendResponsePolicy> _response_policy;
};

} // namespace uskit

#endif // USKIT_BACKEND_SERVICE_H
