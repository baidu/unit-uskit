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

#include "rank_engine.h"
#include "policy/policy_manager.h"
#include "utils.h"

namespace uskit {

RankEngine::RankEngine() {
}

RankEngine::~RankEngine() {
}

int RankEngine::init(const RankEngineConfig& config) {
    for (int i = 0; i < config.rank_size(); ++i) {
        const RankNodeConfig& rank_config = config.rank(i);
        std::shared_ptr<policy::RankPolicy> rank_policy(
            policy::PolicyManager::instance().get_rank_policy(rank_config.rank_policy()));
        if (!rank_policy) {
            LOG(ERROR) << "Rank policy [" << rank_config.rank_policy() << "] not found";
            return -1;
        }

        if (rank_policy->init(rank_config) != 0) {
            LOG(ERROR) << "Failed to init rank rule [" << rank_config.name() << "]";
            return -1;
        }

        _rank_map.emplace(rank_config.name(), std::move(rank_policy));
    }

    return 0;
}

int RankEngine::run(const std::string& name, RankCandidate& rank_candidate,
                    RankResult& rank_result, expression::ExpressionContext& context) const {
    Timer rank_tm("rank_t_ms");
    rank_tm.start();
    if (_rank_map.find(name) != _rank_map.end()) {
        if (_rank_map.at(name)->run(rank_candidate, rank_result, context) != 0) {
            US_LOG(ERROR) << "Failed to run rank rule [" << name << "]";
            rank_tm.stop();
            return -1;
        }
    } else {
        US_LOG(ERROR) << "Rank rule [" << name << "] not found";
        rank_tm.stop();
        return -1;
    }
    rank_tm.stop();

    return 0;
}

} // namespace uskit
