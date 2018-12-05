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

#ifndef USKIT_RANK_ENGINE_H
#define USKIT_RANK_ENGINE_H

#include <string>
#include <unordered_map>
#include "common.h"
#include "dynamic_config.h"
#include "expression/expression.h"
#include "policy/rank_policy.h"

namespace uskit {

// A rank engine manages rank rules of a unified scheduler.
class RankEngine {
public:
    RankEngine();
    RankEngine(RankEngine&&) = default;
    ~RankEngine();

    // Initialize rank engine from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const RankEngineConfig& config);
    // Rank candidates with specified rank rule.
    // Returns 0 on success, -1 otherwise.
    int run(const std::string& name, RankCandidate& rank_candidate,
            RankResult& rank_result, expression::ExpressionContext& context) const;
private:
    std::unordered_map<std::string, std::unique_ptr<policy::RankPolicy>> _rank_map;
};
} // namespace uskit

#endif // USKIT_RANK_ENGINE_H
