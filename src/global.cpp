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
#include "policy/backend/dynamic_policy.h"
#include "policy/backend/host_dyn_http_policy.h"
#include "policy/flow/default_policy.h"
#include "policy/flow/recurrent_policy.h"
#include "policy/flow/global_policy.h"
#include "policy/flow/levels_policy.h"
#include "policy/rank/default_policy.h"

#include "function/builtin.h"
#include "function/str_function.h"

namespace uskit {

void register_function() {
    // Builtin function.
    REGISTER_FUNCTION("get", function::get_value_by_path);
    REGISTER_FUNCTION("set", function::set_value_by_path);
    REGISTER_FUNCTION("index_at", function::get_index_by_key);
    REGISTER_FUNCTION("foreach_get", function::for_each_get_by_path);
    REGISTER_FUNCTION("foreach_set", function::for_each_set_by_path);
    REGISTER_FUNCTION("translate", function::replace_value_by_path_and_dict);
    REGISTER_FUNCTION("array_func", function::array_func);
    REGISTER_FUNCTION("strhas", function::string_contain);
    REGISTER_FUNCTION("substr", function::get_sub_str);
    REGISTER_FUNCTION("normalize", function::normalized);
    REGISTER_FUNCTION("json_encode", function::json_encode);
    REGISTER_FUNCTION("json_decode", function::json_decode);
    REGISTER_FUNCTION("replace_all", function::replace_all);
    REGISTER_FUNCTION("has", function::has_key);
    REGISTER_FUNCTION("len", function::array_length);
    REGISTER_FUNCTION("slice", function::array_slice);
    REGISTER_FUNCTION("int", function::int_value);
    REGISTER_FUNCTION("bool", function::bool_value);
    REGISTER_FUNCTION("md5", function::md5_hash);
    REGISTER_FUNCTION("sha1", function::sha1_hash);
    REGISTER_FUNCTION("time", function::time);
    REGISTER_FUNCTION("hmac_sha1", function::hmac_sha1);
    REGISTER_FUNCTION("base64_encode", function::base64_encode);
    REGISTER_FUNCTION("nonce", function::rand_str);
    REGISTER_FUNCTION("query_encode", function::query_encode);
    REGISTER_FUNCTION("split", function::str_split);
    REGISTER_FUNCTION("str_slice", function::str_slice);
    REGISTER_FUNCTION("join", function::array_join);
    REGISTER_FUNCTION("str_length", function::str_length);
    REGISTER_FUNCTION("str_find", function::str_find);
}

void register_policy() {
    // Request policy.
    REGISTER_REQUEST_POLICY("http_default", policy::backend::HttpRequestPolicy);
    REGISTER_REQUEST_POLICY("redis_default", policy::backend::RedisRequestPolicy);

    REGISTER_REQUEST_POLICY("dynamic_default", policy::backend::DynamicHttpRequestPolicy);
    REGISTER_REQUEST_POLICY("dynamic_host_default", policy::backend::HostDynHttpRequestPolicy);

    // Response policy
    REGISTER_RESPONSE_POLICY("http_default", policy::backend::HttpResponsePolicy);
    REGISTER_RESPONSE_POLICY("redis_default", policy::backend::RedisResponsePolicy);

    REGISTER_RESPONSE_POLICY("dynamic_default", policy::backend::DynamicHttpResponsePolicy);

    REGISTER_RESPONSE_POLICY("dynamic_host_default", policy::backend::HostDynHttpResponsePolicy);

    // Flow policy
    REGISTER_FLOW_POLICY("default", policy::flow::DefaultPolicy);
    REGISTER_FLOW_POLICY("recurrent", policy::flow::AsyncInsideNodePolicy);
    REGISTER_FLOW_POLICY("node_async", policy::flow::AsyncInsideNodePolicy);
    REGISTER_FLOW_POLICY("globalcancel", policy::flow::AsyncGlobalPolicy);
    REGISTER_FLOW_POLICY("global_async", policy::flow::AsyncGlobalPolicy);
    REGISTER_FLOW_POLICY("leveldeliver", policy::flow::CascadeAsyncPolicy);

    // Rank policy
    REGISTER_RANK_POLICY("default", policy::rank::DefaultPolicy);
}

}  // namespace uskit
