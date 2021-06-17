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

#ifndef USKIT_BACKEND_H
#define USKIT_BACKEND_H

#include <string>
#include <vector>
#include <unordered_map>
#include "brpc.h"
#include "dynamic_config.h"

namespace uskit {

// A backend represents a connection to a remote server/cluster, which can be
// used to issue remote service calls(RPC). A backend can be shared by several
// backend services.
// A backend instance is corresponding to a backend section in the `backend.conf`,
// behaviours of a backend can be customized by setting options, i.e. timeout_ms.
// For more info of all available backend options, checkout `docs/config.md`.
class Backend {
public:
    Backend();
    Backend(Backend&&) = default;
    ~Backend();
    // Initialize backend from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const BackendConfig& config);

    // Obtain the channel associated with this backend.
    std::shared_ptr<BRPC_NAMESPACE::Channel> channel() const;
    // Obtain the protocol associated with this backend.
    // Currently supported protocols: HTTP, Redis.
    const BRPC_NAMESPACE::AdaptiveProtocolType protocol() const;
    // Obtain a request config template with given name.
    // Returns nullptr if not found.
    const BackendRequestConfig* request_config(const std::string& name) const;
    // Obtain a response config template with given name.
    // Returns nullptr if not found.
    const BackendResponseConfig* response_config(const std::string& name) const;
    // Obtain all service names associated with this backend.
    const std::vector<std::string>& services() const;
    bool is_dynamic() const;

private:
    // Underlying Channel
    std::shared_ptr<BRPC_NAMESPACE::Channel> _channel;
    // Backend services
    std::vector<std::string> _services;
    // Dynamic requests FLAG, default is false
    bool _is_dynamic;

    // Request config templates
    std::unordered_map<std::string, std::unique_ptr<BackendRequestConfig>>
            _request_config_template_map;
    // Response config templates
    std::unordered_map<std::string, BackendResponseConfig> _response_config_template_map;
};

}  // namespace uskit

#endif  // USKIT_BACKEND_H
