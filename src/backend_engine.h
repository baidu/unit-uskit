// Copyright (c) 2017 Baidu, Inc. All Rights Reserved.
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

#ifndef USKIT_BACKEND_ENGINE_H
#define USKIT_BACKEND_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include "config.pb.h"
#include "expression/expression.h"
#include "backend_service.h"

namespace uskit {

// Forward declaration
namespace policy {
class FlowPolicy;
class FlowPolicyHelper;
}
// A backend engine manages backends and services of a unified scheduler.
class BackendEngine {
public:
    BackendEngine();
    BackendEngine(BackendEngine&&) = default;
    ~BackendEngine();
    // Initialize backend engine from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const BackendEngineConfig& config);
    // Recall specified backend service in parallel.
    // Support cancel operations
    // Return 0 on success, -1 otherwise.
    int run(const std::vector<std::pair<std::string, int> >& recall_services,
            expression::ExpressionContext& context,
            const std::string cancel_order) const;
    int run(const policy::FlowPolicy* flow_policy,
            const FlowRecallConfig* recall_config,
            std::vector<std::shared_ptr<uskit::expression::ExpressionContext> >& context_vec,
            std::shared_ptr<policy::FlowPolicyHelper> helper = nullptr) const;
    // Return number of services
    size_t get_service_size() const;
    size_t get_service_index(const std::string& service_name) const;
    bool has_service(const std::string& service_name) const;

private:
    std::unordered_map<std::string, Backend> _backend_map;
    std::unordered_map<std::string, BackendService> _service_map;
    std::unordered_map<std::string, size_t> _service_context_index;
};

}  // namespace uskit

#endif  // USKIT_BACKEND_ENGINE_H
