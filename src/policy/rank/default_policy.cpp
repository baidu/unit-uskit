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

#include "butil.h"
#include "policy/rank/default_policy.h"
#include "rank_engine.h"

namespace uskit {
namespace policy {
namespace rank {

int DefaultPolicy::init(const RankNodeConfig& config) {
    if (_rank_config.init(config) != 0) {
        LOG(ERROR) << "Failed to initialize rank config";
        return -1;
    }
    return 0;
}

int DefaultPolicy::run(RankCandidate& rank_candidate, RankResult& rank_result,
                       expression::ExpressionContext& context) const {
    if (_rank_config.run(rank_candidate, rank_result, context) != 0) {
        US_LOG(ERROR) << "Failed to evaluate rank config";
        return -1;
    }

    return 0;
}

} // namespace rank
} // namespace policy
} // namespace uskit
