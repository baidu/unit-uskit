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

#include "global.h"
#include "policy/policy_manager.h"
#include "function/function_manager.h"

#include "policy/backend/http_policy.h"
#include "policy/backend/redis_policy.h"
#include "policy/flow/default_policy.h"
#include "policy/rank/default_policy.h"

#include "function/builtin.h"

namespace uskit {

void register_function() {
    // Builtin function.
    REGISTER_FUNCTION("get", function::get_value_by_path);
    REGISTER_FUNCTION("json_encode", function::json_encode);
    REGISTER_FUNCTION("json_decode", function::json_decode);
    REGISTER_FUNCTION("has", function::has_key);
    REGISTER_FUNCTION("len", function::array_length);
    REGISTER_FUNCTION("slice", function::array_slice);
    REGISTER_FUNCTION("int", function::int_value);
    REGISTER_FUNCTION("bool", function::bool_value);
    REGISTER_FUNCTION("md5", function::md5_hash);
    REGISTER_FUNCTION("sha1", function::sha1_hash);
    REGISTER_FUNCTION("time", function::time);
}

void register_policy() {
    // Request policy.
    REGISTER_REQUEST_POLICY("http_default", policy::backend::HttpRequestPolicy);
    REGISTER_REQUEST_POLICY("redis_default", policy::backend::RedisRequestPolicy);

    // Response policy
    REGISTER_RESPONSE_POLICY("http_default", policy::backend::HttpResponsePolicy);
    REGISTER_RESPONSE_POLICY("redis_default", policy::backend::RedisResponsePolicy);

    // Flow policy
    REGISTER_FLOW_POLICY("default", policy::flow::DefaultPolicy);

    // Rank policy
    REGISTER_RANK_POLICY("default", policy::rank::DefaultPolicy);
}

} // namespace uskit
