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

#include "expression/expression.h"
#include "function/function_manager.h"
#include "utils.h"
#include "common.h"

namespace uskit {
namespace expression {

// Preserved keywords.
const std::unordered_set<std::string> ExpressionContext::_keywords = {
    "request", "response", "backend",
    "recall", "output", "next", "export",
    "http_header", "http_query", "http_body",
    "http_uri", "http_method",
    "item", "cond", "rank"
};

ExpressionContext::ExpressionContext(const std::string& name)
    : _name(name), _variables(rapidjson::kObjectType), _parent(nullptr) {
}

ExpressionContext::ExpressionContext(const std::string& name,
                                     rapidjson::Document::AllocatorType& allocator)
    : _name(name), _variables(rapidjson::kObjectType, &allocator), _parent(nullptr) {
    _variables.SetObject();
}

ExpressionContext::ExpressionContext(const std::string& name,
                                     ExpressionContext& context) : _name(name),
    _variables(rapidjson::kObjectType, &context.allocator()), _parent(&context) {
}

void ExpressionContext::set_variable(rapidjson::Value& key, rapidjson::Value& value) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    _variables.AddMember(key, value, allocator());
}

void ExpressionContext::set_variable(const std::string& key, rapidjson::Value& value, bool check_keyword) {
    if (check_keyword) {
        if (_keywords.find(key) != _keywords.end()) {
            US_LOG(WARNING) << key << " is a preserved keyword, overwriting it may cause unexpected results";
        }
    }
    rapidjson::Value variable;
    std::lock_guard<std::mutex> lock(this->_mutex);
    variable.SetString(key.c_str(), key.length(), allocator());
    _variables.AddMember(variable, value, allocator());
}

void ExpressionContext::merge_variable(const std::string& key, rapidjson::Value& value) {
    rapidjson::Value* existing = get_variable(key);
    
    if (existing != nullptr) {
        std::lock_guard<std::mutex> lock(this->_mutex);
        merge_json_objects(*existing, value, allocator());
    } else {
        set_variable(key, value);
    }
}

int ExpressionContext::set_variable_by_path(const std::string& path, rapidjson::Value& value) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    return json_set_value_by_path(path, _variables, value);
}

bool ExpressionContext::erase_variable(const std::string& name) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    return _variables.EraseMember(name.c_str());
}

bool ExpressionContext::has_variable(const std::string& name) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    if (_variables.HasMember(name.c_str())) {
        return true;
    } else {
        return false;
    }
}

rapidjson::Value* ExpressionContext::get_variable(const std::string& name) {
    std::lock_guard<std::mutex> lock(this->_mutex);
    if (_variables.HasMember(name.c_str())) {
        US_DLOG(INFO) << "Has " << name.c_str() << " in _variable: " << this->name().c_str();
        return &_variables[name.c_str()];
    } else if (_parent != nullptr) {
        US_DLOG(INFO) << "Finding " << name.c_str() << " in _parent";
        return _parent->get_variable(name);
    } else {
        return nullptr;
    }
}

rapidjson::Document::AllocatorType& ExpressionContext::allocator() {
    return _variables.GetAllocator();
}

const std::string& ExpressionContext::name() {
    return _name;
}

ExpressionContext* ExpressionContext::parent() {
    return _parent;
}

std::string ExpressionContext::str() {
    std::lock_guard<std::mutex> lock(this->_mutex);
    return json_encode(_variables);
}

Program::Program(Expression* expr) : _expr(expr) {
}

Program::~Program() {
}

std::unique_ptr<Expression> Program::get_expression() {
    return std::move(_expr);
}

Null::Null() {
}

Null::~Null() {
}

int Null::run(ExpressionContext& context, rapidjson::Value& value) {
    value.SetNull();
    return 0;
}

Integer::Integer(int value) : _value(value) {
}

Integer::~Integer() {
}

int Integer::run(ExpressionContext& context, rapidjson::Value& value) {
    value.SetInt(_value);
    return 0;
}

Double::Double(double value) : _value(value) {
}

Double::~Double() {
}

int Double::run(ExpressionContext& context, rapidjson::Value& value) {
    value.SetDouble(_value);
    return 0;
}

String::String(const std::string& str) : _str(str) {
}

String::~String() {
}

int String::run(ExpressionContext& context, rapidjson::Value& value) {
    value.SetString(_str.c_str(), _str.length(), context.allocator());
    return 0;
}

Boolean::Boolean(bool value) : _value(value) {
}

Boolean::~Boolean() {
}

int Boolean::run(ExpressionContext& context, rapidjson::Value& value) {
    value.SetBool(_value);
    return 0;
}

Array::Array(std::vector<Expression*>& array) {
    for (auto & v : array) {
        _array.emplace_back(std::unique_ptr<Expression>(v));
    }
}

Array::~Array() {
}

int Array::run(ExpressionContext& context, rapidjson::Value& value) {
    rapidjson::Document::AllocatorType& allocator = context.allocator();
    value.SetArray();

    for (auto & v : _array) {
        rapidjson::Value item;
        if (v->run(context, item) != 0) {
            return -1;
        }
        value.PushBack(item, allocator);
    }

    return 0;
}

Dict::Dict(KeyValueMap& dict) {
    for (auto & kv : dict) {
        _dict.emplace(kv.first, std::unique_ptr<Expression>(kv.second));
    }
}

Dict::~Dict() {
}

int Dict::run(ExpressionContext& context, rapidjson::Value& value) {
    rapidjson::Document::AllocatorType& allocator = context.allocator();
    value.SetObject();
    for (auto & kv : _dict) {
        rapidjson::Value item_key;
        item_key.SetString(kv.first.c_str(), kv.first.length(), allocator);

        rapidjson::Value item_value;
        if (kv.second->run(context, item_value) != 0) {
            return -1;
        }

        value.AddMember(item_key, item_value, allocator);
    }

    return 0;
}

BinaryExpression::BinaryExpression(const std::string& op, Expression* lhs, Expression* rhs)
    : _op(op), _lhs(lhs), _rhs(rhs) {
}

BinaryExpression::~BinaryExpression() {
}

int BinaryExpression::run(ExpressionContext& context, rapidjson::Value& value) {
    rapidjson::Value lvalue, rvalue;

    if (_lhs->run(context, lvalue) != 0) {
        US_LOG(ERROR) << "[BinaryExpression] Failed to evaluate left expression";
        return -1;
    }

    if (_rhs->run(context, rvalue) != 0) {
        US_LOG(ERROR) << "[BinaryExpression] Failed to evaluate right expression";
        return -1;
    }

    // String catconcatenation.
    if (_op == "+" && lvalue.IsString() && rvalue.IsString()) {
        std::string concat = lvalue.GetString();
        concat += rvalue.GetString();
        value.SetString(concat.c_str(), concat.length(), context.allocator());
        return 0;
    }

    if (_op == "+" && lvalue.IsArray() && rvalue.IsArray()) {
        // Merge two arrays
        rapidjson::Document::AllocatorType& context_alloc = context.allocator();
        value.CopyFrom(lvalue, context_alloc);
        for (auto & v : rvalue.GetArray()) {
            rapidjson::Value copy(v, context.allocator());
            value.PushBack(copy, context.allocator());
        }
        return 0;
    }

    // Set difference.
    if (_op == "-" && lvalue.IsArray() && rvalue.IsArray()) {
        value.SetArray();
        std::unordered_set<std::string> hash;
        for (auto & v : rvalue.GetArray()) {
            std::string v_str = json_encode(v);
            hash.emplace(v_str);
        }
        for (auto & v : lvalue.GetArray()) {
            std::string v_str = json_encode(v);
            if (hash.find(v_str) == hash.end()) {
                rapidjson::Value copy(v, context.allocator());
                value.PushBack(copy, context.allocator());
            }
            hash.emplace(v_str);
        }
        return 0;
    }

    if (_op == "+" || _op == "-" || _op == "*" || _op == "/" ||
        _op == ">" || _op == "<" || _op == ">=" || _op == "<=") {
        // Required numeric parameters.
        if (!lvalue.IsNumber()) {
            US_LOG(ERROR) << "[BinaryExpression] `" << _op
                << "' required left value to be number, but "
                << get_value_type(lvalue) << " found";
            return -1;
        }

        if (!rvalue.IsNumber()) {
            US_LOG(ERROR) << "[BinaryExpression] `" << _op
                << "' required right value to be number, but "
                << get_value_type(rvalue) << " found";
            return -1;
        }

        if (lvalue.IsInt() && rvalue.IsInt()) {
            if (_op == "+") {
                value.SetInt(lvalue.GetInt() + rvalue.GetInt());
            } else if (_op == "-") {
                value.SetInt(lvalue.GetInt() - rvalue.GetInt());
            } else if (_op == "*") {
                value.SetInt(lvalue.GetInt() * rvalue.GetInt());
            } else if (_op == "/") {
                value.SetInt(lvalue.GetInt() / rvalue.GetInt());
            } else if (_op == ">") {
                value.SetBool(lvalue.GetInt() > rvalue.GetInt());
            } else if (_op == "<") {
                value.SetBool(lvalue.GetInt() < rvalue.GetInt());
            } else if (_op == ">=") {
                value.SetBool(lvalue.GetInt() >= rvalue.GetInt());
            } else if (_op == "<="){
                value.SetBool(lvalue.GetInt() <= rvalue.GetInt());
            } else {
                // Never happen.
            }
        } else {
            // Either left or right is double.
            if (_op == "+") {
                value.SetDouble(lvalue.GetDouble() + rvalue.GetDouble());
            } else if (_op == "-") {
                value.SetDouble(lvalue.GetDouble() - rvalue.GetDouble());
            } else if (_op == "*") {
                value.SetDouble(lvalue.GetDouble() * rvalue.GetDouble());
            } else if (_op == "/") {
                value.SetDouble(lvalue.GetDouble() / rvalue.GetDouble());
            } else if (_op == ">") {
                value.SetBool(lvalue.GetDouble() > rvalue.GetDouble());
            } else if (_op == "<") {
                value.SetBool(lvalue.GetDouble() < rvalue.GetDouble());
            } else if (_op == ">=") {
                value.SetBool(lvalue.GetDouble() >= rvalue.GetDouble());
            } else if (_op == "<="){
                value.SetBool(lvalue.GetDouble() <= rvalue.GetDouble());
            } else {
                // Never happen.
            }
        }
    } else if (_op == "||" || _op == "&&") {
        // Required boolean parameters
        if (!lvalue.IsBool() || !rvalue.IsBool()) {
            US_LOG(ERROR) << "Unsupported operand type(s) for " << _op
                << ": " << get_value_type(lvalue) << " and "
                << get_value_type(rvalue);
            return -1;
        }
        if (_op == "||") {
            value.SetBool(lvalue.GetBool() || rvalue.GetBool());
        } else if (_op == "&&") {
            value.SetBool(lvalue.GetBool() && rvalue.GetBool());
        } else {
            // Never happen.
        }
    } else if (_op == "|" || _op == "&") {
        if (lvalue.IsArray() && rvalue.IsArray()) {
            value.SetArray();
            std::unordered_set<std::string> hash;
            for (auto & v : lvalue.GetArray()) {
                std::string v_str = json_encode(v);
                if (hash.find(v_str) == hash.end()) {
                    if (_op == "|") {
                        rapidjson::Value copy(v, context.allocator());
                        value.PushBack(copy, context.allocator());
                    }
                }
                hash.emplace(v_str);
            }
            for (auto & v : rvalue.GetArray()) {
                std::string v_str = json_encode(v);
                if (hash.find(v_str) == hash.end()) {
                    if (_op == "|") {
                        rapidjson::Value copy(v, context.allocator());
                        value.PushBack(copy, context.allocator());
                    }
                } else {
                    if (_op == "&") {
                        rapidjson::Value copy(v, context.allocator());
                        value.PushBack(copy, context.allocator());
                    }
                }
                hash.emplace(v_str);
            }
        } else if (lvalue.IsBool() && rvalue.IsBool()) {
            if (_op == "|") {
                value.SetBool(lvalue.GetBool() | rvalue.GetBool());
            } else if (_op == "&") {
                value.SetBool(lvalue.GetBool() & rvalue.GetBool());
            } else {
                // Never happen.
            }
        } else if (lvalue.IsInt() && rvalue.IsInt()) {
            if (_op == "|") {
                value.SetInt(lvalue.GetInt() | rvalue.GetInt());
            } else if (_op == "&") {
                value.SetInt(lvalue.GetInt() & rvalue.GetInt());
            } else {
                // Never happen.
            }
        } else {
            US_LOG(ERROR) << "Unsupported operand type(s) for " << _op
                << ": " << get_value_type(lvalue) << " and "
                << get_value_type(rvalue);
            return -1;
        }
    } else {
        // Operator == and !=
        if (_op == "==") {
            value.SetBool(lvalue == rvalue);
        } else if (_op == "!=") {
            value.SetBool(lvalue != rvalue);
        } else {
            // Never happen.
        }
    }

    return 0;
}

TernaryExpression::TernaryExpression(Expression* cond, Expression* lhs, Expression* rhs)
    : _cond(cond), _lhs(lhs), _rhs(rhs) {
}

TernaryExpression::~TernaryExpression() {
}

int TernaryExpression::run(ExpressionContext& context, rapidjson::Value& value) {
    rapidjson::Value cond_value;
    if (_cond->run(context, cond_value) != 0) {
        US_LOG(ERROR) << "[TernaryExpression] Failed to evaluate condition expression";
        return -1;
    }

    if (!cond_value.IsBool()) {
        US_LOG(ERROR) << "[TernaryExpression] required condition value to be bool, but "
            << get_value_type(cond_value) << " found";
        return -1;
    }

    if (cond_value.GetBool()) {
        rapidjson::Value lvalue;
        if (_lhs->run(context, lvalue) != 0) {
            US_LOG(ERROR) << "[TernaryExpression] Failed to evaluate left expression";
            return -1;
        }
        rapidjson::Document::AllocatorType& context_alloc = context.allocator();
        value.CopyFrom(lvalue, context_alloc);
    } else {
        rapidjson::Value rvalue;
        if (_rhs->run(context, rvalue) != 0) {
            US_LOG(ERROR) << "[TernaryExpression] Failed to evaluate right expression";
            return -1;
        }
        rapidjson::Document::AllocatorType& context_alloc = context.allocator();
        value.CopyFrom(rvalue, context_alloc);
    }

    return 0;
}

NotExpression::NotExpression(Expression* expr) : _expr(expr) {
}

NotExpression::~NotExpression() {
}

int NotExpression::run(ExpressionContext& context, rapidjson::Value& value) {
    rapidjson::Value negate_value;

    if (_expr->run(context, negate_value) != 0) {
        US_LOG(ERROR) << "[NotExpression] Failed to evaluate expression";
        return -1;
    }
    if (!negate_value.IsBool()) {
        US_LOG(ERROR) << "[NotExpression] required value to be bool, but "
            << get_value_type(negate_value) << " found";
        return -1;
    }
    value.SetBool(!negate_value.GetBool());

    return 0;
}

CallExpression::CallExpression(const std::string& func_name,
               std::vector<Expression*>& args) : _func_name(func_name), _next(nullptr) {
    for (auto & arg : args) {
        _args.emplace_back(arg);
    }
}

CallExpression::~CallExpression() {
}

int CallExpression::set_next(CallExpression* next) {
    _next.reset(next);
    return 0;
}

int CallExpression::run(ExpressionContext& context, rapidjson::Value& value) {
    rapidjson::Value v;
    return run(context, value, v);
}

int CallExpression::run(ExpressionContext& context, rapidjson::Value& value, const rapidjson::Value& input) {
    rapidjson::Document::AllocatorType& allocator = context.allocator();

    rapidjson::Value arg_values;
    arg_values.SetArray();
    if (!input.IsNull()) {
        rapidjson::Value copy_value(input, allocator);
        arg_values.PushBack(copy_value, allocator);
    }

    for (auto & arg : _args) {
        rapidjson::Value arg_value;
        if (arg->run(context, arg_value) != 0) {
            return -1;
        }
        arg_values.PushBack(arg_value, allocator);
    }

    rapidjson::Document middle_value(&allocator);
    // Find function by name and run with evaluated args
    US_DLOG(INFO) << "run function [" << _func_name << "]";
    int ret = function::FunctionManager::instance().call_function(_func_name,
                                                                  arg_values,
                                                                  middle_value);
    if (ret != 0) {
        US_LOG(ERROR) << "Failed to run function [" << _func_name << "]";
        return -1;
    }

    rapidjson::Document return_value(&allocator);
    if (_next) {
        ret = _next->run(context, return_value, middle_value);
        if (ret != 0) {
            US_LOG(ERROR) << "Failed to run the function after [" << _func_name << "]";
            return -1;
        }
    } else {
        return_value.Swap(middle_value);
    }

    value.Swap(return_value);

    return 0;
}

VariableExpression::VariableExpression(const std::string& name) : _name(name) {
}

VariableExpression::~VariableExpression() {
}

int VariableExpression::run(ExpressionContext& context, rapidjson::Value& value) {
    // Search for variable.
    rapidjson::Value* variable = context.get_variable(_name);
    rapidjson::Document::AllocatorType& context_alloc = context.allocator();
    if (variable == nullptr) {
        US_LOG(ERROR) << "Variable [" << _name << "] undefined";
        return -1;
    }
    //std::string output_str = "search for variable [" + _name + "], context: " + context.name() + " " + context.str();
    //US_DLOG(ERROR) << output_str;
    //if (&context_alloc == nullptr) {
    //    US_DLOG(ERROR) << "Variabel [" << _name << "] context allocator NULL";
    //}
    US_DLOG(INFO) << json_encode(*variable);
    value = rapidjson::Value(*variable, context_alloc);

    return 0;
}

} // namespace expression
} // namespace uskit
