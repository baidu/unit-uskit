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

#ifndef USKIT_FLOW_ENGINE_H
#define USKIT_FLOW_ENGINE_H

#include "common.h"
#include "dynamic_config.h"
#include "policy/flow_policy.h"

namespace uskit {

// A flow engine manages chat flow of a unified scheduler.
// Flow engine may interact with backend engine to recall services and rank recalled
// bot skills as needed.
class FlowEngine {
public:
    FlowEngine();
    FlowEngine(FlowEngine&&) = default;
    ~FlowEngine();

    // Initialize rank engine from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const std::string& root_dir, const std::string& usid);
    int init(const USConfig& config);
    int init_by_message(const FlowEngineConfig& flow_config);
    int member_init(const USConfig& config);
    int member_init(const std::string& root_dir, const std::string& usid);
    int member_init_by_message(
            const BackendEngineConfig& backend_config,
            const RankEngineConfig& rank_config);
    // Run chat flow with user reqeust and generate response.
    // Returns 0 on success, -1 otherwise.
    int run(USRequest& request, USResponse& response) const;

private:
    std::unique_ptr<policy::FlowPolicy> _flow_policy;
    std::shared_ptr<BackendEngine> _backend_engine;
    std::shared_ptr<RankEngine> _rank_engine;
    rapidjson::Document _intervene_config_json;
    std::string _curr_dir;
};

}  // namespace uskit

#endif  // USKIT_FLOW_ENGINE_H
