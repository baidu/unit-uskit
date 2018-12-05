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

#include "flow_engine.h"
#include "policy/policy_manager.h"

namespace uskit {

FlowEngine::FlowEngine() {
}

FlowEngine::~FlowEngine() {
}

int FlowEngine::init(const FlowEngineConfig& config) {
    _flow_policy = std::unique_ptr<policy::FlowPolicy>(
        policy::PolicyManager::instance().get_flow_policy(config.flow_policy()));
    if (!_flow_policy) {
        LOG(ERROR) << "Flow policy [" << config.flow_policy() << "] not found";
        return -1;
    }

    if (_flow_policy->init(config.flow()) != 0) {
        LOG(ERROR) << "Failed to initialize flow policy [" << config.flow_policy() << "]";
        return -1;
    }

    return 0;
}

int FlowEngine::run(USRequest& request, USResponse& response,
                    const BackendEngine* backend_engine,
                    const RankEngine* rank_engine) const {
    if (_flow_policy->run(request, response, backend_engine, rank_engine) != 0) {
        return -1;
    }
    return 0;
}

} // namespace uskit
