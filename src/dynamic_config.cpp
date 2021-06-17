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

#include <algorithm>
#include "butil.h"
#include "dynamic_config.h"
#include "expression/driver.h"
#include "utils.h"
#include "backend_engine.h"
#include "rank_engine.h"
#include "policy/flow_policy.h"

namespace uskit {

int KEMap::init(const google::protobuf::RepeatedPtrField<KVE>& kve_list) {
    expression::Driver driver;

    for (const auto& kve : kve_list) {
        Expr expr;
        // Key expression pair.
        if (kve.has_expr()) {
            if (driver.parse(kve.key(), kve.expr()) != 0) {
                LOG(ERROR) << "Failed to parse expression of [" << kve.key() << "]";
                return -1;
            }
            expr = driver.get_expression();
        } else if (kve.has_value()) {
            // Key value pair.
            expr = Expr(new expression::String(kve.value()));
        } else {
            LOG(ERROR) << "Required value or expression of [" << kve.key() << "] to be set";
            return -1;
        }
        _ke_map.emplace(kve.key(), std::move(expr));
        _key_order.emplace_back(kve.key());
    }
    StringCompare cmp(uskit::KVE_DELIMETER, uskit::KVE_IS_ROOT2PATH);
    std::stable_sort(_key_order.begin(), _key_order.end(), cmp);

    return 0;
}

int KEMap::run_def(expression::ExpressionContext& context) const {
    // Evaluate definitions in order
    for (const auto& key : _key_order) {
        rapidjson::Value value;
        US_DLOG(INFO) << "evaluate expression of [" << key << "]";
        if (_ke_map.at(key)->run(context, value) != 0) {
            US_LOG(ERROR) << "Failed to evaluate expression of [" << key << "]";
            return -1;
        }
        context.set_variable_by_path(key, value);
    }
    return 0;
}

int KEMap::run(expression::ExpressionContext& context, rapidjson::Document& doc) const {
    // Set null if empty.
    if (_ke_map.size() == 0) {
        doc.SetNull();
        return 0;
    }
    doc.SetObject();
    for (const auto& key : _key_order) {
        rapidjson::Value value;
        US_DLOG(INFO) << "evaluate expression of [" << key << "]";
        if (_ke_map.at(key)->run(context, value) != 0) {
            US_LOG(ERROR) << "Failed to evaluate expression of [" << key << "]";
            return -1;
        }
        json_set_value_by_path(key, doc, value);
    }
    return 0;
}

bool KEMap::empty() const {
    return _ke_map.empty();
}

int KEVec::init(const google::protobuf::RepeatedPtrField<std::string>& expr_list) {
    expression::Driver driver;
    for (const auto& expr : expr_list) {
        if (driver.parse("", expr) != 0) {
            LOG(ERROR) << "Failed to parse expression";
            return -1;
        }
        _ke_vec.emplace_back(driver.get_expression());
    }
    return 0;
}

int KEVec::init(const std::vector<std::string>& expr_list) {
    expression::Driver driver;
    for (const auto& expr : expr_list) {
        if (driver.parse("", expr) != 0) {
            LOG(ERROR) << "Failed to parse expression";
            return -1;
        }
        _ke_vec.emplace_back(driver.get_expression());
    }
    return 0;
}

int KEVec::logical_and(expression::ExpressionContext& context, bool& value) const {
    value = true;
    for (const auto& ke : _ke_vec) {
        rapidjson::Value v;
        if (ke->run(context, v) != 0) {
            US_LOG(ERROR) << "Failed to evaluate expression";
            return -1;
        }
        if (!v.IsBool()) {
            US_LOG(ERROR) << "Expression didn't evaluate to boolean";
            return -1;
        }
        if (!v.GetBool()) {
            value = false;
            break;
        }
    }
    return 0;
}

int KEVec::logical_or(expression::ExpressionContext& context, bool& value) const {
    value = false;
    for (const auto& ke : _ke_vec) {
        rapidjson::Value v;
        if (ke->run(context, v) != 0) {
            US_LOG(ERROR) << "Failed to evaluate expression";
            return -1;
        }
        if (!v.IsBool()) {
            US_LOG(ERROR) << "Expression didn't evaluate to boolean";
            return -1;
        }
        if (v.GetBool()) {
            value = true;
            break;
        }
    }
    return 0;
}

int KEVec::run(expression::ExpressionContext& context, rapidjson::Value& value) const {
    value.SetArray();
    for (const auto& ke : _ke_vec) {
        rapidjson::Value v;
        if (ke->run(context, v) != 0) {
            US_LOG(ERROR) << "Failed to evaluate expression";
            return -1;
        }
        US_DLOG(INFO) << "v value: " << json_encode(v);
        value.PushBack(v, context.allocator());
    }
    return 0;
}

int BackendRequestConfig::init(
        const RequestConfig& config,
        const BackendRequestConfig* template_config) {
    if (_definition.init(config.def()) != 0) {
        return -1;
    }
    _template = template_config;

    return 0;
}

int BackendRequestConfig::run(expression::ExpressionContext& context) const {
    if (_definition.run_def(context) != 0) {
        US_LOG(ERROR) << "Failed to evaluate definition";
        return -1;
    }

    if (_template != nullptr) {
        if (_template->run(context) != 0) {
            US_LOG(ERROR) << "Failed to generate request config template";
            return -1;
        }
    }

    return 0;
}

int HttpRequestConfig::init(
        const RequestConfig& config,
        const BackendRequestConfig* template_config) {
    if (BackendRequestConfig::init(config, template_config) != 0) {
        return -1;
    }

    if (_http_header.init(config.http_header()) != 0) {
        return -1;
    }
    if (_http_query.init(config.http_query()) != 0) {
        return -1;
    }
    if (_http_body.init(config.http_body()) != 0) {
        return -1;
    }
    if (config.has_host_ip_port()) {
        expression::Driver driver;
        if (driver.parse("", config.host_ip_port()) != 0) {
            LOG(ERROR) << "Failed to parse expression";
            return -1;
        }
        _host_ip_port = driver.get_expression();
    }
    if (config.has_http_method()) {
        _http_method = Expr(new expression::String(config.http_method()));
    }
    if (config.has_http_uri()) {
        _http_uri = Expr(new expression::String(config.http_uri()));
    }

    return 0;
}

int HttpRequestConfig::run(expression::ExpressionContext& context) const {
    if (BackendRequestConfig::run(context) != 0) {
        return -1;
    }
    rapidjson::Document::AllocatorType& allocator = context.allocator();

    if (_http_method) {
        rapidjson::Value value;
        if (_http_method->run(context, value) != 0) {
            return -1;
        }
        context.set_variable("http_method", value);
    }
    if (_http_uri) {
        rapidjson::Value value;
        if (_http_uri->run(context, value) != 0) {
            return -1;
        }
        context.set_variable("http_uri", value);
    }
    if (_host_ip_port) {
        rapidjson::Value value;
        if (_host_ip_port->run(context, value) != 0) {
            return -1;
        }
        context.set_variable("host_ip_port", value);
    }

    rapidjson::Document http_header_doc(&allocator);
    if (_http_header.run(context, http_header_doc) != 0) {
        US_LOG(ERROR) << "Failed to evaluate http header";
        return -1;
    }
    if (!http_header_doc.IsNull()) {
        context.merge_variable("http_header", http_header_doc);
    }

    rapidjson::Document http_query_doc(&allocator);
    if (_http_query.run(context, http_query_doc) != 0) {
        US_LOG(ERROR) << "Failed to evaluate http query";
        return -1;
    }
    if (!http_query_doc.IsNull()) {
        context.merge_variable("http_query", http_query_doc);
    }

    rapidjson::Document http_body_doc(&allocator);
    if (_http_body.run(context, http_body_doc) != 0) {
        US_LOG(ERROR) << "Failed to evaluate http body";
        return -1;
    }
    if (!http_body_doc.IsNull()) {
        context.merge_variable("http_body", http_body_doc);
    }

    return 0;
}

int DynamicHttpRequestConfig::init(
        const RequestConfig& config,
        const BackendRequestConfig* template_config) {
    if (HttpRequestConfig::init(config, template_config) != 0) {
        return -1;
    }
    if (_dynamic_args.init(config.dynamic_args()) != 0) {
        return -1;
    }
    if (config.has_dynamic_args_node()) {
        _dynamic_args_node = Expr(new expression::String(config.dynamic_args_node()));
    }
    if (config.has_dynamic_args_path()) {
        _dynamic_args_path = Expr(new expression::String(config.dynamic_args_path()));
    }

    return 0;
}

int DynamicHttpRequestConfig::run(expression::ExpressionContext& context) const {
    if (HttpRequestConfig::run(context) != 0) {
        return -1;
    }
    rapidjson::Document::AllocatorType& allocator = context.allocator();

    if (_dynamic_args_node) {
        rapidjson::Value value;
        if (_dynamic_args_node->run(context, value) != 0) {
            return -1;
        }
        context.set_variable("dynamic_args_node", value);
    }
    if (_dynamic_args_path) {
        rapidjson::Value value;
        if (_dynamic_args_path->run(context, value) != 0) {
            return -1;
        }
        context.set_variable("dynamic_args_path", value);
    }

    rapidjson::Document dynamic_args_doc(&allocator);
    if (_dynamic_args.run(context, dynamic_args_doc) != 0) {
        US_LOG(ERROR) << "Failed to evaluate dynamic args";
        return -1;
    }
    if (!dynamic_args_doc.IsNull()) {
        context.set_variable("dynamic_args", dynamic_args_doc);
    }

    return 0;
}

int RedisCommand::init(const RequestConfig::RedisCommandConfig& config) {
    _op = config.op();
    if (_arg.init(config.arg()) != 0) {
        return -1;
    }
    return 0;
}

int RedisCommand::run(expression::ExpressionContext& context, rapidjson::Value& value) const {
    value.SetObject();
    rapidjson::Value arg_value;
    if (_arg.run(context, arg_value) != 0) {
        return -1;
    }
    rapidjson::Value op_value;
    op_value.SetString(_op.c_str(), _op.length(), context.allocator());
    value.AddMember("op", op_value, context.allocator());
    rapidjson::Value arg_str_value(rapidjson::kArrayType);
    for (auto& v : arg_value.GetArray()) {
        if (v.IsString()) {
            arg_str_value.PushBack(v, context.allocator());
        } else {
            // Serialize value if not string.
            std::string str = json_encode(v);
            rapidjson::Value str_value;
            str_value.SetString(str.c_str(), str.length(), context.allocator());
            arg_str_value.PushBack(str_value, context.allocator());
        }
    }
    value.AddMember("arg", arg_str_value, context.allocator());
    return 0;
}

int RedisRequestConfig::init(
        const RequestConfig& config,
        const BackendRequestConfig* template_config) {
    if (BackendRequestConfig::init(config, template_config) != 0) {
        return -1;
    }
    for (const auto& cmd_config : config.redis_cmd()) {
        RedisCommand cmd;
        if (cmd.init(cmd_config) != 0) {
            return -1;
        }
        _redis_command.emplace_back(std::move(cmd));
    }
    return 0;
}

int RedisRequestConfig::run(expression::ExpressionContext& context) const {
    if (BackendRequestConfig::run(context) != 0) {
        return -1;
    }
    rapidjson::Value redis_cmd_value(rapidjson::kArrayType);
    for (const auto& cmd : _redis_command) {
        rapidjson::Value v;
        if (cmd.run(context, v) != 0) {
            return -1;
        }
        redis_cmd_value.PushBack(v, context.allocator());
    }
    context.set_variable("redis_cmd", redis_cmd_value);
    return 0;
}

int BaseIfConfig::init(const IfConfig& config) {
    if (_definition.init(config.def()) != 0) {
        return -1;
    }
    if (_condition.init(config.cond()) != 0) {
        return -1;
    }
    if (_output.init(config.output()) != 0) {
        return -1;
    }
    return 0;
}

int BaseIfConfig::run(expression::ExpressionContext& context) const {
    // US_LOG(WARNING) << "start generate response if config";
    rapidjson::Document::AllocatorType& allocator = context.allocator();

    bool cond_value = true;
    // US_DLOG(WARNING) << "start logical and";
    if (_condition.logical_and(context, cond_value) != 0) {  // CORE
        US_LOG(ERROR) << "Failed to evaluate condition";
        return -1;
    }
    context.set_variable("cond", rapidjson::Value().SetBool(cond_value));
    if (!cond_value) {
        US_LOG(INFO) << "Condition expression evaluate to false, skip";
        return 0;
    }
    // US_DLOG(WARNING) << "start run_def";
    if (_definition.run_def(context) != 0) {
        US_LOG(ERROR) << "Failed to evaluate definition";
        return -1;
    }
    // US_DLOG(WARNING) << "start output";
    rapidjson::Document output_doc(&allocator);
    if (_output.run(context, output_doc) != 0) {  // CORE
        US_LOG(ERROR) << "Failed to evaluate output";
        return -1;
    }
    if (!output_doc.IsNull()) {
        context.merge_variable("output", output_doc);
    }

    return 0;
}

int BackendResponseIfConfig::run(expression::ExpressionContext& context) const {
    return BaseIfConfig::run(context);
}

int BackendResponseConfig::init(
        const ResponseConfig& config,
        const BackendResponseConfig* template_config) {
    if (_definition.init(config.def()) != 0) {
        return -1;
    }
    if (_output.init(config.output()) != 0) {
        return -1;
    }

    for (const auto& if_config : config.if_()) {
        BackendResponseIfConfig response_if_config;
        if (response_if_config.init(if_config) != 0) {
            return -1;
        }
        _if.emplace_back(std::move(response_if_config));
    }

    _template = template_config;

    return 0;
}

int BackendResponseConfig::run(expression::ExpressionContext& context) const {
    rapidjson::Document::AllocatorType& allocator = context.allocator();
    if (_definition.run_def(context) != 0) {
        US_LOG(ERROR) << "Failed to evaluate definition";
        return -1;
    }

    if (_template != nullptr) {
        if (_template->run(context) != 0) {
            US_LOG(ERROR) << "Failed to generate response config template";
            return -1;
        }
    }

    // Choose if branch
    for (const auto& if_block : _if) {
        expression::ExpressionContext if_context("response if block", context);
        if (if_block.run(if_context) != 0) {
            US_LOG(ERROR) << "Failed to run if block";
            continue;  // changed
        }

        rapidjson::Value* if_cond = if_context.get_variable("cond");
        if (if_cond == nullptr) {
            US_LOG(ERROR) << "If condition not found";
            return -1;
        }
        if (if_cond->IsBool() && if_cond->GetBool()) {
            rapidjson::Value* if_output = if_context.get_variable("output");
            if (if_output != nullptr) {
                context.merge_variable("output", *if_output);
            }

            return 0;
        }
    }

    // Else branch
    rapidjson::Document output_doc(&allocator);
    if (_output.run(context, output_doc) != 0) {
        US_LOG(ERROR) << "Failed to evaluate output";
        return -1;
    }
    if (!output_doc.IsNull()) {
        context.merge_variable("output", output_doc);
    }
    US_LOG(WARNING) << "output is NULL";
    return 0;
}

Compare::Compare(const std::vector<bool>* desc) : _desc(desc) {}

bool Compare::operator()(const rapidjson::Value& a, const rapidjson::Value& b) {
    for (rapidjson::SizeType i = 0; i < a["features"].Size(); ++i) {
        if (a["features"][i].IsNull() && b["features"][i].IsNull()) {
            return false;
        } else if (a["features"][i].IsNull()) {
            if (0.0 < b["features"][i].GetDouble()) {
                return !_desc->at(i);
            } else if (0.0 > b["features"][i].GetDouble()) {
                return _desc->at(i);
            }
        } else if (b["features"][i].IsNull()) {
            if (a["features"][i].GetDouble() < 0.0) {
                return !_desc->at(i);
            } else if (a["features"][i].GetDouble() > 0.0) {
                return _desc->at(i);
            }
        } else {
            if (a["features"][i].GetDouble() < b["features"][i].GetDouble()) {
                return !_desc->at(i);
            } else if (a["features"][i].GetDouble() > b["features"][i].GetDouble()) {
                return _desc->at(i);
            }
        }
    }
    return false;
}

StringCompare::StringCompare(const std::string& delimeter, bool is_root2leaf) :
        _delimeter(delimeter), _is_root2leaf(is_root2leaf) {}

bool StringCompare::operator()(const std::string& a, const std::string& b) {
    std::string a_trim;
    BUTIL_NAMESPACE::TrimString(a, _delimeter, &a_trim);
    std::string b_trim;
    BUTIL_NAMESPACE::TrimString(b, _delimeter, &b_trim);
    auto a_levels = std::count(a_trim.begin(), a_trim.end(), _delimeter.c_str()[0]);
    auto b_levels = std::count(b_trim.begin(), b_trim.end(), _delimeter.c_str()[0]);
    if (a_levels == b_levels && a_levels == 0) {
        if (a_trim == "") {
            return _is_root2leaf;
        } else if (b_trim == "") {
            return !_is_root2leaf;
        }
    } else if (a_levels < b_levels) {
        return _is_root2leaf;
    } else if (a_levels > b_levels) {
        return !_is_root2leaf;
    }
    return false;
}

int RankConfig::init(const RankNodeConfig& config) {
    int order_size = config.order_size();
    for (int i = 0; i < order_size; ++i) {
        const std::string order = config.order(i);
        // Multiple candidates of same priority.
        if (order.find(',') != std::string::npos) {
            std::vector<std::string> split;
            BUTIL_NAMESPACE::SplitString(order, ',', &split);
            for (auto& s : split) {
                _order.emplace(s, order_size - i);
            }
        } else {
            _order.emplace(order, order_size - i);
        }
    }
    if (!_order.empty()) {
        _desc.push_back(true);
    }

    std::vector<std::string> sort_by_expr_list;
    for (const auto& sort_by : config.sort_by()) {
        sort_by_expr_list.emplace_back(sort_by.expr());
        _desc.push_back(sort_by.desc() == 1);
    }

    if (_sort_by.init(sort_by_expr_list) != 0) {
        return -1;
    }

    return 0;
}

int RankConfig::run(
        RankCandidate& rank_candidate,
        RankResult& rank_result,
        expression::ExpressionContext& context) const {
    rapidjson::Document::AllocatorType& allocator = context.allocator();

    US_DLOG(INFO) << "candidates: " << json_encode(rank_candidate);
    rapidjson::Value* backend = context.get_variable("backend");
    if (backend == nullptr) {
        return 0;
    }
    std::vector<rapidjson::Value> rank_candidate_features;
    for (auto& v : rank_candidate.GetArray()) {
        std::string name = v.GetString();
        if (backend->HasMember(v)) {
            rapidjson::Value features;
            features.SetObject();
            features.AddMember("features", rapidjson::Value().SetArray(), allocator);
            // Add features by priority.
            if (!_order.empty() && _order.find(name) != _order.end()) {
                features["features"].PushBack(
                        rapidjson::Value().SetInt(_order.at(name)), allocator);
            }

            US_DLOG(INFO) << "features: " << json_encode(features);

            rapidjson::Value item((*backend)[v], allocator);
            int index = -1;
            if (item.IsArray()) {
                for (auto& item_v : item.GetArray()) {
                    rapidjson::Value ele_features(features, allocator);
                    index += 1;
                    rapidjson::Value ele_item(item_v, allocator);
                    if (context.has_variable("item")) {
                        context.erase_variable("item");
                    }
                    context.set_variable("item", ele_item);
                    US_DLOG(INFO) << "context value: "
                                  << json_encode(*(context.get_variable("item")));
                    rapidjson::Value features_value;
                    if (_sort_by.run(context, features_value) != 0) {
                        // Failed to extract features, skip candidate
                        US_LOG(WARNING) << "Failed to evaluate sort_by"
                                        << " of [" << name << "], skip";
                        continue;
                    }
                    US_DLOG(INFO) << "features value: " << json_encode(features_value);
                    for (auto& f : features_value.GetArray()) {
                        ele_features["features"].PushBack(f, allocator);
                    }
                    // Successfully collect all features.
                    if (ele_features["features"].Size() == _desc.size()) {
                        std::string rank_item_name =
                                std::string(v.GetString()) + "/" + std::to_string(index);
                        US_DLOG(INFO) << "Rank item name: " << rank_item_name;
                        rapidjson::Value rank_name;
                        rank_name.SetString(
                                rank_item_name.c_str(), rank_item_name.length(), allocator);
                        ele_features.AddMember("name", rank_name, allocator);
                        US_DLOG(INFO) << "features: " << json_encode(ele_features);
                        rank_candidate_features.emplace_back(std::move(ele_features));
                    }
                }
            } else {
                if (context.has_variable("item")) {
                    context.erase_variable("item");
                }
                context.set_variable("item", item);
                US_DLOG(INFO) << "context value: " << json_encode(*(context.get_variable("item")));
                rapidjson::Value features_value;
                if (_sort_by.run(context, features_value) != 0) {
                    // Failed to extract features, skip candidate
                    US_LOG(WARNING) << "Failed to evaluate sort_by"
                                    << " of [" << name << "], skip";
                    continue;
                }
                US_DLOG(INFO) << "features value: " << json_encode(features_value);
                for (auto& f : features_value.GetArray()) {
                    features["features"].PushBack(f, allocator);
                }

                // Successfully collect all features.
                if (features["features"].Size() == _desc.size()) {
                    features.AddMember("name", v, allocator);
                    US_DLOG(INFO) << "features: " << json_encode(features);
                    rank_candidate_features.emplace_back(std::move(features));
                }
            }
        } else {
            US_LOG(WARNING) << "Rank candidate [" << name << "] not found, skip";
        }
    }

    // Sort by features
    Compare cmp(&_desc);
    std::sort(rank_candidate_features.begin(), rank_candidate_features.end(), cmp);

    for (auto& v : rank_candidate_features) {
        rank_result.PushBack(v["name"], allocator);
    }

    return 0;
}

int CallIdsVecThreadSafe::init(std::vector<BRPC_NAMESPACE::CallId>::size_type length) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    _is_ready = false;
    for (size_t i = 0; i != length; ++i) {
        _call_ids.push_back(BRPC_NAMESPACE::CallId());
        _multi_call_ids.push_back(std::vector<BRPC_NAMESPACE::CallId>(0));
    }
    return 0;
}

bool CallIdsVecThreadSafe::get_is_ready() {
    std::lock_guard<std::mutex> lock(this->_mutex);
    return _is_ready;
}

int CallIdsVecThreadSafe::set_call_id(
        std::vector<BRPC_NAMESPACE::CallId>::size_type index,
        BRPC_NAMESPACE::CallId service_call_id) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    if (index >= _call_ids.size()) {
        US_LOG(ERROR) << "service index exceed callid array";
        return -1;
    }
    if (_call_ids[index] != INVALID_BTHREAD_ID) {
        US_LOG(ERROR) << "call id of service at " << index << "already set";
        return -1;
    }
    if (_is_ready) {
        US_LOG(WARNING) << "quit condition is satisfied, no need to request";
        return 1;
    }
    _call_ids[index] = service_call_id;
    return 0;
}

int CallIdsVecThreadSafe::set_call_id(
        std::vector<std::vector<BRPC_NAMESPACE::CallId>>::size_type index,
        const std::vector<BRPC_NAMESPACE::CallId>& call_ids_vec) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    if (index >= _multi_call_ids.size()) {
        US_LOG(ERROR) << "service index exceed callid array";
        return -1;
    }
    if (_multi_call_ids[index].size() != 0) {
        US_LOG(ERROR) << "call id of service at " << index << "already set";
        return -1;
    }
    if (_is_ready) {
        US_LOG(WARNING) << "quit condition is satisfied, no need to request";
        return 1;
    }
    _multi_call_ids[index] = call_ids_vec;
    return 0;
}

int CallIdsVecThreadSafe::set_ready_to_quite() {
    std::lock_guard<std::mutex> lock(this->_mutex);
    _is_ready = true;
    return 0;
}

int CallIdsVecThreadSafe::quit_flow() {
    std::lock_guard<std::mutex> lock(this->_mutex);
    for (auto call_id : _call_ids) {
        if (call_id != INVALID_BTHREAD_ID) {
            BRPC_NAMESPACE::StartCancel(call_id);
            US_DLOG(INFO) << "quit call id: " << call_id;
        }
    }
    for (auto call_id_vec : _multi_call_ids) {
        if (call_id_vec.size() != 0) {
            for (auto call_id : call_id_vec) {
                BRPC_NAMESPACE::StartCancel(call_id);
                US_DLOG(INFO) << "quit call id: " << call_id;
            }
        }
    }
    _is_ready = true;
    return 0;
}

int FlowIfConfig::init(const IfConfig& config) {
    if (BaseIfConfig::init(config) != 0) {
        return -1;
    }
    if (config.has_next()) {
        _next = Expr(new expression::String(config.next()));
        US_DLOG(INFO) << "set _next succss";
    } else {
        US_DLOG(INFO) << "_next is missing";
    }

    return 0;
}

int FlowIfConfig::run(expression::ExpressionContext& context) const {
    BaseIfConfig::run(context);
    if (_next) {
        rapidjson::Value value;
        if (_next->run(context, value) != 0) {
            return -1;
        }
        context.set_variable("next", value);
        US_DLOG(INFO) << "set if block next";
    } else {
        US_DLOG(INFO) << "_next is false";
    }

    return 0;
}

int FlowGlobalCancelConfig::init(const FlowNodeConfig::GlobalCancelConfig& config) {
    if (_definition.init(config.def()) != 0) {
        return -1;
    }
    if (_condition.init(config.cond()) != 0) {
        return -1;
    }
    return 0;
}

bool FlowGlobalCancelConfig::run(expression::ExpressionContext& context) const {
    US_LOG(INFO) << "start generate flow quit config";

    bool cond_value = true;
    if (_condition.logical_and(context, cond_value) != 0) {
        US_LOG(ERROR) << "Failed to evaluate condition";
        return -1;
    }
    context.set_variable("cond", rapidjson::Value().SetBool(cond_value));
    return cond_value;
}

int FlowRankConfig::init(const FlowNodeConfig::RankConfig& config) {
    expression::Driver driver;

    _top_k = config.top_k();

    if (driver.parse("input", config.input()) != 0) {
        LOG(ERROR) << "Failed to parse expression of [input]";
        return -1;
    }
    _input = driver.get_expression();

    _rule = config.rule();

    return 0;
}

int FlowRankConfig::run(
        const policy::FlowPolicy* flow_policy,
        expression::ExpressionContext& context) const {
    rapidjson::Document::AllocatorType& allocator = context.allocator();

    rapidjson::Value input_value;
    if (_input) {
        if (_input->run(context, input_value) != 0) {
            US_LOG(ERROR) << "Failed to evaluate rank input";
            return -1;
        }
    }
    US_DLOG(INFO) << "input: " << json_encode(input_value);

    // Rank input
    RankResult rank_result;
    rank_result.SetArray();
    if (flow_policy->rank_run(_rule, input_value, rank_result, context) != 0) {
        US_LOG(ERROR) << "Failed to rank";
        return -1;
    }
    US_DLOG(INFO) << "rank result: " << json_encode(rank_result);

    // Select top K result
    if (_top_k > 0 && rank_result.Size() > _top_k) {
        RankResult top_k_rank_result;
        top_k_rank_result.SetArray();
        for (size_t i = 0; i < _top_k; ++i) {
            top_k_rank_result.PushBack(rank_result[i], allocator);
        }
        rank_result = std::move(top_k_rank_result);
    }
    if (context.has_variable("rank")) {
        context.erase_variable("rank");
    }
    context.set_variable("rank", rank_result);

    return 0;
}

int FlowRecallConfig::init(
        const FlowNodeConfig::RecallConfig& recall_config,
        const std::string& flow_name) {
    US_LOG(INFO) << "cancel order: " << recall_config.cancel_order();
    _cancel_order = recall_config.cancel_order();
    _flow_name = flow_name;
    for (auto& service : recall_config.recall_service()) {
        if (service.has_next_flow()) {
            if (service.next_flow() == flow_name) {
                US_LOG(ERROR) << "Self jump not allowed";
                return -1;
            }
            _recall_next.emplace(service.recall_name(), service.next_flow());
        }
        if (service.has_priority()) {
            _recall_services.push_back(
                    std::pair<std::string, int>(service.recall_name(), service.priority()));
        } else {
            _recall_services.push_back(std::pair<std::string, int>(service.recall_name(), 0));
        }
    }
    return 0;
}

int FlowRecallConfig::init(
        const google::protobuf::RepeatedPtrField<FlowNodeConfig::DeliverConfig>& deliver_configs,
        const std::string& flow_name) {
    _cancel_order = "GLOBAL_CANCEL";
    _flow_name = flow_name;
    for (const auto& deliver_config : deliver_configs) {
        std::string service_name = deliver_config.classify_service();
        _recall_next.emplace(service_name, service_name + FlowDeliverConfig::get_suffix());
        _recall_services.push_back(std::pair<std::string, int>(service_name, 0));
    }
    return 0;
}

int FlowRecallConfig::init(
        const google::protobuf::RepeatedPtrField<std::string>& recall,
        const std::string& flow_name) {
    _flow_name = flow_name;
    _cancel_order = "";
    for (const auto& service : recall) {
        _recall_services.push_back(std::pair<std::string, int>(service, 0));
    }
    return 0;
}

int FlowRecallConfig::get_intervene_service(
        expression::ExpressionContext& context,
        std::string& service) const {
    if (_intervene_config) {
        std::string source = "";
        if (_intervene_config->evaluate_source(context, source) != 0) {
            US_LOG(ERROR) << "Falied to evaluate source of intervene config";
            return -1;
        }
        if (_intervene_config->evaluate_target_service(source, service) != 0) {
            US_LOG(ERROR) << "Failed to find target of intervene config";
            return -1;
        }
    }
    return 0;
}

int FlowRecallConfig::get_intervene_flow(
        expression::ExpressionContext& context,
        std::string& service) const {
    service.clear();
    if (_intervene_config) {
        std::string source = "";
        if (_intervene_config->evaluate_source(context, source) != 0) {
            US_LOG(ERROR) << "Falied to evaluate source of intervene config";
            return -1;
        }
        if (_intervene_config->evaluate_target_flow(source, service) != 0) {
            US_LOG(ERROR) << "Failed to find target of intervene config";
            return -1;
        }
        US_DLOG(INFO) << "service: [" << service << "]";
    }
    return 0;
}

int FlowRecallConfig::run(
        const policy::FlowPolicy* flow_policy,
        expression::ExpressionContext& context) const {
    std::string intervene_result = "";
    if (get_intervene_service(context, intervene_result) != 0) {
        return -1;
    }
    if (intervene_result != "") {
        std::vector<std::pair<std::string, int>> tmp_recall_services = {
                std::make_pair(intervene_result, 0)};
        if (flow_policy->backend_run(tmp_recall_services, context, _cancel_order) != 0) {
            US_LOG(ERROR) << "Recall with cancel option failed";
            return -1;
        }
    } else {
        if (flow_policy->backend_run(_recall_services, context, _cancel_order) != 0) {
            US_LOG(ERROR) << "Recall with cancel option failed";
            return -1;
        }
    }
    return 0;
}

int FlowRecallConfig::run(
        const policy::FlowPolicy* flow_policy,
        std::vector<std::shared_ptr<uskit::expression::ExpressionContext>>& context_vec,
        std::shared_ptr<policy::FlowPolicyHelper> helper) const {
    if (flow_policy->backend_run(this, context_vec, helper) != 0) {
        US_LOG(ERROR) << "Recall with flow map failed";
        return -1;
    }
    return 0;
}

const std::string FlowDeliverConfig::_SUFFIX = "__NEXT";

int FlowDeliverConfig::init(const FlowNodeConfig::DeliverConfig& d_config) {
    _class_num = d_config.class_num();
    _classify_service = d_config.classify_service();
    // The type of `RepeatedField.size()` is int
    if (d_config.deliver_service().size() != static_cast<int>(_class_num - 1)) {
        US_LOG(ERROR) << "number of classes and delivered service unmatched";
        return -1;
    }

    _threshold_vec.SetArray();
    for (auto service : d_config.deliver_service()) {
        _deliver_service_vec.push_back(service.service_name());
        expression::Driver driver;
        Expr expr;
        if (driver.parse("", service.threshold()) != 0) {
            US_LOG(ERROR) << "Failed to parse expression threshold";
            return -1;
        }
        rapidjson::Value value;
        expr = driver.get_expression();
        uskit::expression::ExpressionContext context("");
        if (expr->run(context, value) != 0) {
            US_LOG(ERROR) << "Failed to parse threshold for service [" << service.service_name()
                          << "]";
            return -1;
        }
        _threshold_vec.PushBack(value, _toy_document.GetAllocator());
    }
    return 0;
}

int FlowInterveneConfig::init_by_intervene_file(const InterveneFileConfig& file_config) {
    for (auto rule : file_config.rule()) {
        std::string pattern = rule.pattern();
        std::string target = rule.target();
        bool is_entire = rule.entire_match();
        bool is_service = rule.service_intervene();
        std::string pattern_key = "";
        try {
            boost::regex re(pattern);
            if (is_entire == true) {
                _match_list.push_back(re);
                pattern_key = re.str();
            } else {
                _search_list.push_back(re);
                pattern_key = re.str() + "__SEARCH";
            }
            if (is_service == true) {
                _pattern_service_map.emplace(pattern_key, target);
            } else {
                _pattern_flow_map.emplace(pattern_key, target);
            }
        } catch (boost::bad_expression e) {
            US_LOG(WARNING) << "patten: [" << pattern << "] compile error, " << e.what();
            continue;
        }
    }

    US_DLOG(INFO) << "#match: " << _match_list.size() << " #search: " << _search_list.size()
                  << " #service: " << _pattern_service_map.size()
                  << " #flow: " << _pattern_flow_map.size();
    return 0;
}

int FlowInterveneConfig::parse_source(const std::string& source) {
    expression::Driver driver;
    if (driver.parse("", source) != 0) {
        US_LOG(ERROR) << "Failed to parse expression: " << source;
        return -1;
    }
    _source = driver.get_expression();
    return 0;
}

int FlowInterveneConfig::init(const InterveneConfig& config, const InterveneFileConfig& file_config) {
    if (parse_source(config.source()) != 0) {
        return -1;
    }
    if (init_by_intervene_file(file_config) != 0) {
        return -1;
    }
    return 0;
}

int FlowInterveneConfig::evaluate_source(
        expression::ExpressionContext& context,
        std::string& out_source) const {
    rapidjson::Value value;
    if (_source->run(context, value) != 0) {
        US_LOG(ERROR) << "evaluate source failed during intervene";
        return -1;
    } else if (!value.IsString()) {
        US_LOG(ERROR) << "Source should be STRING, " << get_value_type(value) << " is given";
        return -1;
    } else {
        out_source = value.GetString();
    }
    return 0;
}

int FlowInterveneConfig::evaluate_target_service(
        const std::string& in_source,
        std::string& out_target) const {
    for (auto re : _match_list) {
        if (_pattern_service_map.find(re.str()) != _pattern_service_map.end() &&
            boost::regex_match(in_source, re)) {
            US_DLOG(INFO) << "RE: " << re.str() << "matched in [" << in_source << "]";
            out_target = _pattern_service_map.at(re.str());
            return 0;
        }
    }
    for (auto re : _search_list) {
        if (_pattern_service_map.find(re.str() + "__SEARCH") != _pattern_service_map.end() &&
            boost::regex_search(in_source, re)) {
            US_DLOG(INFO) << "RE: " << re.str() << "searched in [" << in_source << "]";
            out_target = _pattern_service_map.at(re.str() + "__SEARCH");
            return 0;
        }
    }
    return 0;
}

int FlowInterveneConfig::evaluate_target_flow(const std::string& in_source, std::string& out_target)
        const {
    for (auto re : _match_list) {
        if (_pattern_flow_map.find(re.str()) != _pattern_flow_map.end() &&
            boost::regex_match(in_source, re)) {
            US_DLOG(INFO) << "RE: " << re.str() << "matched in [" << in_source << "]";
            out_target = _pattern_flow_map.at(re.str());
            return 0;
        }
    }
    for (auto re : _search_list) {
        if (_pattern_flow_map.find(re.str() + "__SEARCH") != _pattern_flow_map.end() &&
            boost::regex_search(in_source, re)) {
            US_DLOG(INFO) << "RE: " << re.str() << "searched in [" << in_source << "]";
            out_target = _pattern_flow_map.at(re.str() + "__SEARCH");
            US_DLOG(INFO) << "out_target: [" << out_target << "]";
            return 0;
        }
    }
    return 0;
}

int FlowRecallConfig::intervene_init(const InterveneConfig& config, const InterveneFileConfig& file_config) {
    _intervene_config = std::make_unique<FlowInterveneConfig>();
    if (_intervene_config->init(config, file_config) != 0) {
        US_LOG(ERROR) << "Intervene Config parse error in flow [" << _flow_name;
        return -1;
    }
    return 0;
}

int FlowConfig::init(FlowDeliverConfig&& d_config) {
    _name = d_config.get_flow_name();
    _recall_config = std::unique_ptr<FlowRecallConfig>(new FlowRecallConfig());
    FlowNodeConfig tmp_config = FlowNodeConfig();
    for (auto iter = d_config._deliver_service_vec.begin();
         iter != d_config._deliver_service_vec.end();
         ++iter) {
        tmp_config.add_recall(*iter);
    }
    _recall_config->init(tmp_config.recall(), _name);
    _deliver_config = std::make_unique<FlowDeliverConfig>(std::move(d_config));
    return 0;
}

int FlowConfig::init(const FlowNodeConfig& config) {
    _name = config.name();
    if (_definition.init(config.def()) != 0) {
        return -1;
    }
    if (_output.init(config.output()) != 0) {
        return -1;
    }

    for (const auto& d_config : config.deliver_config()) {
        FlowDeliverConfig flow_deliver_config;
        if (flow_deliver_config.init(d_config) != 0) {
            return -1;
        }
        _deliver_map.emplace(flow_deliver_config.get_flow_name(), std::move(flow_deliver_config));
    }

    _recall_config = std::unique_ptr<FlowRecallConfig>(new FlowRecallConfig());
    if (config.has_recall_config()) {
        if (_recall_config->init(config.recall_config(), _name) != 0) {
            return -1;
        }
    } else if (config.deliver_config().size() > 0) {
        if (_recall_config->init(config.deliver_config(), _name) != 0) {
            return -1;
        }
    } else {
        if (_recall_config->init(config.recall(), _name) != 0) {
            return -1;
        }
    }

    for (const auto& d_config : config.deliver_config()) {
        FlowDeliverConfig flow_deliver_config;
        if (flow_deliver_config.init(d_config) != 0) {
            return -1;
        }
        _deliver_map.emplace(flow_deliver_config.get_flow_name(), std::move(flow_deliver_config));
    }

    for (const auto& if_config : config.if_()) {
        FlowIfConfig flow_if_config;
        if (flow_if_config.init(if_config) != 0) {
            return -1;
        }
        _if.emplace_back(std::move(flow_if_config));
    }

    for (const auto& rank_config : config.rank()) {
        FlowRankConfig flow_rank_config;
        if (flow_rank_config.init(rank_config) != 0) {
            return -1;
        }
        _rank.emplace_back(std::move(flow_rank_config));
    }

    if (config.has_next()) {
        _next = Expr(new expression::String(config.next()));
    }

    return 0;
}

int FlowConfig::set_quit_conifg(const FlowNodeConfig::GlobalCancelConfig& config) {
    if (_quit_config.init(config) != 0) {
        return -1;
    }
    return 0;
}

bool FlowConfig::run_quit_config(expression::ExpressionContext& context) const {
    return _quit_config.run(context);
}

int FlowConfig::recall(
        const policy::FlowPolicy* flow_policy,
        expression::ExpressionContext& context) const {
    if (_definition.run_def(context) != 0) {
        US_LOG(ERROR) << "Failed to evaluate definition";
        return -1;
    }
    if (_recall_config->run(flow_policy, context) != 0) {
        US_LOG(ERROR) << "Failed to recall services";
        return -1;
    }

    return 0;
}

std::vector<std::string> FlowConfig::get_recall_service_list() const {
    std::vector<std::string> service_list;
    for (auto iter : _recall_config->get_recall_services()) {
        service_list.push_back(iter.first);
    }
    return service_list;
}

int FlowConfig::set_intervene_config(const InterveneConfig& config, const InterveneFileConfig file_config) {
    return _recall_config->intervene_init(config, file_config);
}

int FlowConfig::get_intervene_flow(expression::ExpressionContext& context, std::string& target_flow)
        const {
    if (_recall_config->get_intervene_flow(context, target_flow) != 0) {
        US_LOG(ERROR) << "Evaluate next intervene flow failed in [" << _name << "]";
        return -1;
    }
    US_DLOG(INFO) << "target_flow: [" << target_flow << "]";
    return 0;
}

int FlowConfig::recall(
        const policy::FlowPolicy* flow_policy,
        std::vector<std::shared_ptr<uskit::expression::ExpressionContext>>& context_vec,
        std::shared_ptr<policy::FlowPolicyHelper> helper) const {
    for (auto iter : _recall_config->get_recall_services()) {
        if (!flow_policy->has_service(iter.first)) {
            US_LOG(ERROR) << "Service name [" << iter.first << "] not defined";
            return -1;
        }
        size_t service_index = flow_policy->get_service_index(iter.first);
        if (_definition.run_def(*(context_vec[service_index])) != 0) {
            US_LOG(ERROR) << "Failed to evaluate definition";
            return -1;
        }
    }
    if (_recall_config->run(flow_policy, context_vec, helper) != 0) {
        if (helper->_call_ids_ptr) {
            US_LOG(ERROR) << "Failed to recall service of global cancel policy";
        } else {
            US_LOG(ERROR) << "Failed to recall services";
        }
        return -1;
    }
    return 0;
}

int FlowConfig::recall_run_def(expression::ExpressionContext& context) const {
    if (_definition.run_def(context) != 0) {
        US_LOG(ERROR) << "Failed to evalute definition";
        return -1;
    }
    return 0;
}

int FlowConfig::rank(const policy::FlowPolicy* flow_policy, expression::ExpressionContext& context)
        const {
    US_DLOG(INFO) << "Evaluating flow rank block";

    for (const auto& rank_block : _rank) {
        if (rank_block.run(flow_policy, context) != 0) {
            US_LOG(ERROR) << "Failed to run rank block";
            return -1;
        }
    }

    return 0;
}

int FlowConfig::output(expression::ExpressionContext& context) const {
    US_DLOG(INFO) << "Evaluating flow output block";
    rapidjson::Document::AllocatorType& allocator = context.allocator();
    // Choose if branch
    for (const auto& if_block : _if) {
        expression::ExpressionContext if_context("flow if block", context);
        if (if_block.run(if_context) != 0) {
            US_LOG(ERROR) << "Failed to run if block";
            // return -1;
            continue;
        }

        rapidjson::Value* if_cond = if_context.get_variable("cond");
        if (if_cond == nullptr) {
            US_LOG(ERROR) << "If condition not found";
            return -1;
        }
        if (if_cond->IsBool() && if_cond->GetBool()) {
            rapidjson::Value* if_output = if_context.get_variable("output");
            if (if_output != nullptr) {
                context.merge_variable("output", *if_output);
            }

            rapidjson::Value* if_next = if_context.get_variable("next");
            if (if_next != nullptr) {
                US_DLOG(INFO) << "context set var: next";
                context.set_variable("next", *if_next);
            }
            return 0;
        }
    }

    rapidjson::Document output_doc(&allocator);
    if (_output.run(context, output_doc) != 0) {
        US_LOG(ERROR) << "Failed to evaluate output";
        return -1;
    }
    if (!output_doc.IsNull()) {
        context.merge_variable("output", output_doc);
    }

    if (_next) {
        rapidjson::Value value;
        if (_next->run(context, value) != 0) {
            return -1;
        }
        context.set_variable("next", value);
    }

    return 0;
}

std::vector<std::string> FlowConfig::filterout_by_response(const rapidjson::Value& response) const {
    std::vector<std::string> filterout;
    rapidjson::Document toy_document;
    if (response.GetType() != rapidjson::kArrayType) {
        US_LOG(WARNING) << "Classify service response should be Array, ["
                        << get_value_type(response) << "] is given";
        return filterout;
    }
    if (response.GetArray().Empty()) {
        US_LOG(WARNING) << "Response array is EMPTY";
        return filterout;
    }
    rapidjson::Value response_array(rapidjson::kArrayType);
    for (auto& iter : response.GetArray()) {
        rapidjson::Value iter_value(iter, toy_document.GetAllocator());
        response_array.PushBack(iter_value, toy_document.GetAllocator());
    }

    if (response_array.Size() == 1 && response_array[0].GetType() != rapidjson::kStringType) {
        US_LOG(WARNING) << "Only string type is supported when there is exactly one element in "
                           "response array, ["
                        << get_value_type(response_array[0]) << "] is given";
        return filterout;
    } else if (response_array.Size() == 1) {
        rapidjson::Value tmp_value(response_array[0], toy_document.GetAllocator());
        for (size_t idx = 1; idx < _deliver_config->_class_num; ++idx) {
            response_array.PushBack(tmp_value, toy_document.GetAllocator());
        }
    }
    if (_deliver_config->_class_num != response_array.Size()) {
        US_LOG(WARNING) << "Classify service response length is different from class_num";
        return filterout;
    }
    size_t response_offset = 1;
    for (size_t idx = response_offset; idx < _deliver_config->_class_num; ++idx) {
        size_t t_idx = idx - response_offset;
        std::string service_name = _deliver_config->_deliver_service_vec[t_idx];

        if (response_array[idx].GetType() != _deliver_config->_threshold_vec[t_idx].GetType()) {
            US_LOG(WARNING) << "response result [" << idx << "] type "
                            << get_value_type(response_array[idx])
                            << " is conflict with threshold type "
                            << get_value_type(_deliver_config->_threshold_vec[t_idx]);
            filterout.push_back(service_name);
        } else if (
                response_array[idx].GetType() == rapidjson::kStringType &&
                std::strcmp(
                        response_array[idx].GetString(),
                        _deliver_config->_threshold_vec[t_idx].GetString()) != 0) {
            US_DLOG(INFO) << "Response: " << response_array[idx].GetString()
                          << " threshold: " << _deliver_config->_threshold_vec[t_idx].GetString();
            US_DLOG(INFO) << "PUSH " << service_name;
            filterout.push_back(service_name);
        } else if (
                response_array[idx].GetType() == rapidjson::kNumberType &&
                response_array[idx].GetDouble() <
                        _deliver_config->_threshold_vec[t_idx].GetDouble()) {
            US_DLOG(INFO) << "Response: " << response_array[idx].GetDouble()
                          << " threshold: " << _deliver_config->_threshold_vec[t_idx].GetDouble();
            US_DLOG(INFO) << "PUSH " << service_name;
            filterout.push_back(service_name);
        }
    }
    return filterout;
}

}  // namespace uskit
