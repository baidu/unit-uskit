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

#ifndef USKIT_DYNAMIC_CONFIG_H
#define USKIT_DYNAMIC_CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>
#include "config.pb.h"
#include "common.h"
#include "expression/expression.h"

namespace uskit {

typedef std::unique_ptr<expression::Expression> Expr;

// Key-expression mapping.
class KEMap {
public:
    KEMap() {}
    KEMap(KEMap&&) = default;
    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const google::protobuf::RepeatedPtrField<KVE>& kve_list);
    // Evaluate all key-expression pairs and inject evaluated results into given context.
    // Returns 0 on success, -1 otherwise.
    int run_def(expression::ExpressionContext& context) const;
    // Evaluate all key-expression pairs.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context, rapidjson::Document& doc) const;
    // Empty test.
    bool empty() const;

private:
    std::unordered_map<std::string, Expr> _ke_map;
    std::vector<std::string> _key_order;
};

// Expresssion array.
class KEVec {
public:
    KEVec() {}
    KEVec(KEVec&&) = default;
    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const google::protobuf::RepeatedPtrField<std::string>& expr_list);
    // Initialize from vector of expression strings.
    // Returns 0 on success, -1 otherwise.
    int init(const std::vector<std::string>& expr_list);
    // Evaluate all expressions in array and perform a logical and between all
    // evaluated results.
    // Note: this operation requires type of every evaluated result to be bool.
    // Returns 0 on success, -1 otherwise.
    int logical_and(expression::ExpressionContext& context, bool& value) const;
    // Evaluate all expressions in array and perform a logical or between all
    // evaluated results.
    // Note: this operation requires type of every evaluated result to be bool.
    // Returns 0 on success, -1 otherwise.
    int logical_or(expression::ExpressionContext& context, bool& value) const;
    // Evaluate all expression in array and return the evaluated array.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context, rapidjson::Value& value) const;

private:
    std::vector<Expr> _ke_vec;
};

// Dynamic configuration of backend request.
class BackendRequestConfig {
public:
    BackendRequestConfig() {}
    virtual ~BackendRequestConfig() {}
    BackendRequestConfig(BackendRequestConfig&&) = default;
    // Initialize from configuration and template(optional).
    // Returns 0 on success, -1 otherwise.
    virtual int init(
            const RequestConfig& config,
            const BackendRequestConfig* template_config = nullptr);
    // Evaluate all expressions within given context and generate backend
    // request configuration.
    // Returns 0 on success, -1 otherwise.
    virtual int run(expression::ExpressionContext& context) const;

private:
    const BackendRequestConfig* _template;
    // Local definitions.
    KEMap _definition;
};

// Dynamic configuration of HTTP request.
class HttpRequestConfig : public BackendRequestConfig {
public:
    HttpRequestConfig() {}
    ~HttpRequestConfig() {}
    HttpRequestConfig(HttpRequestConfig&&) = default;
    // Initialize from configuration and template(optional).
    // Returns 0 on success, -1 otherwise.
    int init(const RequestConfig& config, const BackendRequestConfig* template_config);
    // Evaluate all expressions within given context and generate HTTP
    // request configuration.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context) const;

private:
    Expr _http_method;
    Expr _http_uri;

    KEMap _http_header;
    KEMap _http_query;
    KEMap _http_body;
};

// Dynamic configuration of dynamic HTTP request.
class DynamicHttpRequestConfig : public BackendRequestConfig {
public:
    DynamicHttpRequestConfig() {}
    ~DynamicHttpRequestConfig() {}
    DynamicHttpRequestConfig(DynamicHttpRequestConfig&&) = default;
    // Initialize from configuration and template(optional).
    // Returns 0 on success, -1 otherwise.
    int init(const RequestConfig& config, const BackendRequestConfig* template_config);
    // Evaluate all expressions within given context and generate HTTP
    // request configuration.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context) const;

private:
    Expr _http_method;
    Expr _http_uri;
    Expr _dynamic_args_node;
    Expr _dynamic_args_path;

    KEMap _http_header;
    KEMap _http_query;
    KEMap _http_body;
    KEMap _dynamic_args;
};

// Dynamic configuration of Redis command.
class RedisCommand {
public:
    RedisCommand() {}
    RedisCommand(RedisCommand&&) = default;
    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const RequestConfig::RedisCommandConfig& config);
    // Evaluate all expressions within given context and generate Redis
    // command configuration.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context, rapidjson::Value& value) const;

private:
    // Redis operator.
    std::string _op;
    // Operation arguments.
    KEVec _arg;
};

// Dynamic configuration of Redis request.
class RedisRequestConfig : public BackendRequestConfig {
public:
    RedisRequestConfig() {}
    ~RedisRequestConfig() {}
    RedisRequestConfig(RedisRequestConfig&&) = default;
    // Initialize from configuration and template(optional).
    // Returns 0 on success, -1 otherwise.
    int init(const RequestConfig& config, const BackendRequestConfig* template_config);
    // Evaluate all expressions within given context and generate Redis
    // request configuration.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context) const;

private:
    std::vector<RedisCommand> _redis_command;
};

// Dynamic configuration of backend if block.
class BackendResponseIfConfig {
public:
    BackendResponseIfConfig() {}
    BackendResponseIfConfig(BackendResponseIfConfig&&) = default;
    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const ResponseConfig::IfConfig& config);
    // Evaluate all expressions within given context and generate backend
    // if block.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context) const;

private:
    // Local definitions.
    KEMap _definition;
    // If conditions.
    KEVec _condition;
    KEMap _output;
};

// Dynamic configuration of backend response.
class BackendResponseConfig {
public:
    BackendResponseConfig() {}
    BackendResponseConfig(BackendResponseConfig&&) = default;
    // Initialize from configuration and template(optional).
    // Returns 0 on success, -1 otherwise.
    int init(const ResponseConfig& config, const BackendResponseConfig* template_config = nullptr);
    // Evaluate all expressions within given context and generate backend
    // response configuration.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context) const;

private:
    const BackendResponseConfig* _template;

    // Local definitions.
    KEMap _definition;
    // If blocks.
    std::vector<BackendResponseIfConfig> _if;
    KEMap _output;
};

// Custom comparison class for ranking.
class Compare {
public:
    Compare(const std::vector<bool>* desc);
    // Custom comparison operator.
    bool operator()(const rapidjson::Value& a, const rapidjson::Value& b);

private:
    const std::vector<bool>* _desc;
};

// Forward declaration
class RankEngine;

// Dynamic configuration of rank rule.
class RankConfig {
public:
    RankConfig() {}
    RankConfig(RankConfig&&) = default;
    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const RankNodeConfig& config);
    // Evaluate all expressions and rank candidates with respect to `order' and
    // evaluated result of `sort_by'.
    // Returns 0 on success, -1 otherwise.
    int run(RankCandidate& rank_candidate,
            RankResult& rank_result,
            expression::ExpressionContext& context) const;

private:
    KEVec _sort_by;
    std::vector<bool> _desc;
    std::unordered_map<std::string, int> _order;
};

// Dynamic configuration of flow if block.
class FlowIfConfig {
public:
    FlowIfConfig() {}
    FlowIfConfig(FlowIfConfig&&) = default;

    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const FlowNodeConfig::IfConfig& config);
    // Evaluate all expressions within given context and generate flow
    // if block.
    // Returns 0 on success, -1 otherwise.
    int run(expression::ExpressionContext& context) const;

private:
    // Local definitions.
    KEMap _definition;
    // If conditions.
    KEVec _condition;
    KEMap _output;
    // Next flow node.
    Expr _next;
};

// vector for call ids w.r.t services. thread-safe
class CallIdsVecThreadSafe {
public:
    int set_ready_to_quite();
    int set_call_id(
            std::vector<BRPC_NAMESPACE::CallId>::size_type index,
            BRPC_NAMESPACE::CallId call_id);
    int set_call_id(
            std::vector<std::vector<BRPC_NAMESPACE::CallId>>::size_type index,
            const std::vector<BRPC_NAMESPACE::CallId>& call_ids_vec);
    bool get_is_ready();
    int init(std::vector<BRPC_NAMESPACE::CallId>::size_type length);
    int quit_flow();

private:
    bool _is_ready;
    std::vector<BRPC_NAMESPACE::CallId> _call_ids;
    std::mutex _mutex;
    std::vector<std::vector<BRPC_NAMESPACE::CallId>> _multi_call_ids;
};

// Dynamic configuration of flow if block.
class FlowGlobalCancelConfig {
public:
    FlowGlobalCancelConfig() {}
    FlowGlobalCancelConfig(FlowGlobalCancelConfig&&) = default;

    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const FlowNodeConfig::GlobalCancelConfig& config);
    // Evaluate all expressions within given context and generate flow
    // if block.
    // Returns 0 on success, -1 otherwise.
    bool run(expression::ExpressionContext& context) const;

private:
    // Local definitions.
    KEMap _definition;
    // If conditions.
    KEVec _condition;
    KEMap _output;
    // Next flow node.
    Expr _next;
};

// Dynamic configuration of flow rank block.
class FlowRankConfig {
public:
    FlowRankConfig() {}
    FlowRankConfig(FlowRankConfig&&) = default;

    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const FlowNodeConfig::RankConfig& config);
    // Evaluate all expressions within given context and generate flow
    // rank block.
    // Returns 0 on success, -1 otherwise.
    int run(const RankEngine* rank_engine, expression::ExpressionContext& context) const;

private:
    // Retain top k rank results.
    size_t _top_k;
    // Array of rank candidate names.
    Expr _input;
    // Name of rank rule.
    std::string _rule;
};

// Forward declaration
class BackendEngine;
class FlowConfig;

class FlowRecallConfig {
public:
    FlowRecallConfig() {}
    FlowRecallConfig(FlowRecallConfig&&) = default;

    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const FlowNodeConfig::RecallConfig& config, const std::string& flow_name);
    // Evaluate all expressions within given context and generate flow
    // recall block.
    // Returns 0 on success, -1 otherwise.
    int run(const BackendEngine* backend_engine, expression::ExpressionContext& context) const;

    int run(const BackendEngine* backend_engine,
            std::vector<expression::ExpressionContext*>& context,
            const std::unordered_map<std::string, FlowConfig>* flow_map,
            const RankEngine* rank_engine,
            std::shared_ptr<CallIdsVecThreadSafe> ids_ptr = nullptr) const;

    const std::string get_cancel_order() const {
        return _cancel_order;
    }
    const std::vector<std::pair<std::string, int>> get_recall_services() const {
        return _recall_services;
    }
    const std::unordered_map<std::string, std::string> get_recall_next() const {
        return _recall_next;
    }
    const std::string get_flow_name() const {
        return _flow_name;
    }

private:
    // recall config obj
    std::string _cancel_order;
    std::vector<std::pair<std::string, int>> _recall_services;
    std::unordered_map<std::string, std::string> _recall_next;
    std::string _flow_name;
};

// Dynamic configuration of flow node.
class FlowConfig {
public:
    FlowConfig() {}
    FlowConfig(FlowConfig&&) = default;
    // Initialize from configuration.
    // Returns 0 on success, -1 otherwise.
    int init(const FlowNodeConfig& config);

    // Evaluate definitions and recall specified backend services in parallel.
    // Returns 0 on success, -1 otherwise.
    int recall(const BackendEngine* backend_engine, expression::ExpressionContext& context) const;

    int recall(
            const BackendEngine* backend_engine,
            std::vector<expression::ExpressionContext*>& context,
            const std::unordered_map<std::string, FlowConfig>* flow_map,
            const RankEngine* rank_engine,
            std::shared_ptr<CallIdsVecThreadSafe> ids_ptr = nullptr) const;

    // assign global quit config to flow config.
    int set_quit_conifg(const FlowNodeConfig::GlobalCancelConfig& config);
    // Evaluate quit config condtion.
    bool run_quit_config(expression::ExpressionContext& context) const;
    // Evaluate definitions for single context
    int recall_run_def(expression::ExpressionContext& context) const;
    // Evaluate flow rank block and rank candidates with specified rank rule.
    // Returns 0 on success, -1 otherwise.
    int rank(const RankEngine* rank_engine, expression::ExpressionContext& context) const;
    // Evaluate flow if block and output blocks to generate output result.
    // Returns 0 on success, -1 otherwise.
    int output(expression::ExpressionContext& context) const;
    // Get all services' name in recall config
    std::vector<std::string> get_recall_service_list() const;

private:
    std::string _name;
    // Local definitions.
    KEMap _definition;
    // Recall services.
    std::vector<std::string> _recall;
    // Flow if blocks.
    std::vector<FlowIfConfig> _if;
    // Global cancel block.
    FlowGlobalCancelConfig _quit_config;
    // Flow rank blocks.
    std::vector<FlowRankConfig> _rank;
    // Flow recall block.
    std::unique_ptr<FlowRecallConfig> _recall_config;
    KEMap _output;
    // Next flow node.
    Expr _next;
};

}  // namespace uskit

#endif  // USKIT_DYNAMIC_CONFIG_H
