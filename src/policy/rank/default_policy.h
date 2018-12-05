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

#ifndef USKIT_POLICY_RANK_DEFAULT_POLICY_H
#define USKIT_POLICY_RANK_DEFAULT_POLICY_H

#include "policy/rank_policy.h"
#include "dynamic_config.h"

namespace uskit {
namespace policy {
namespace rank {

// Default rank policy.
class DefaultPolicy : public RankPolicy {
public:
    int init(const RankNodeConfig& config);
    int run(RankCandidate& rank_candidate, RankResult& rank_result,
            expression::ExpressionContext& context) const;
private:
    RankConfig _rank_config;
};

} // namespace rank
} // namespace policy
} // namespace uskit

#endif // USKIT_POLICY_RANK_DEFAULT_POLICY_H
