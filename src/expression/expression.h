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

#ifndef USKIT_EXPRESSION_H
#define USKIT_EXPRESSION_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <rapidjson/document.h>
#include <mutex>
#include <thread>

namespace uskit {
namespace expression {

// Context for expression evaluation.
class ExpressionContext {
public:
    ExpressionContext(const std::string& name);
    ExpressionContext(const std::string& name, rapidjson::Document::AllocatorType& allocator);
    ExpressionContext(const std::string& name, ExpressionContext& parent);

    void set_variable(rapidjson::Value& key, rapidjson::Value& value);
    void set_variable(const std::string& key, rapidjson::Value& value, bool check_keyword = false);
    void merge_variable(const std::string& key, rapidjson::Value& value);
    int set_variable_by_path(const std::string& path, rapidjson::Value& value);
    bool erase_variable(const std::string& name);
    bool has_variable(const std::string& name);
    rapidjson::Value* get_variable(const std::string& name);

    rapidjson::Document::AllocatorType& allocator();
    // Context name.
    const std::string& name();
    // Outter context.
    ExpressionContext* parent();
    // Get serialized JSON string of this context.
    std::string str();
    std::mutex _outer_mutex;

private:
    std::string _name;
    rapidjson::Document _variables;
    ExpressionContext* _parent;
    std::mutex _mutex;

    const static std::unordered_set<std::string> _keywords;
};

// Base AST node class of all expressions.
class Expression {
public:
    virtual ~Expression() = default;
    // Evaluate expression within given context.
    virtual int run(ExpressionContext& context, rapidjson::Value& value) = 0;
};

// Expression AST container.
class Program {
public:
    Program(Expression* expr);
    ~Program();
    std::unique_ptr<Expression> get_expression();

private:
    std::unique_ptr<Expression> _expr;
};

// Null expression.
class Null : public Expression {
public:
    Null();
    ~Null();
    int run(ExpressionContext& context, rapidjson::Value& value);
};

// Integer expression.
class Integer : public Expression {
public:
    Integer(int value);
    ~Integer();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    int _value;
};

// Double expression.
class Double : public Expression {
public:
    Double(double value);
    ~Double();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    double _value;
};

// String expression.
class String : public Expression {
public:
    String(const std::string& str);
    ~String();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    std::string _str;
};

// Boolean expression.
class Boolean : public Expression {
public:
    Boolean(bool value);
    ~Boolean();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    bool _value;
};

// Array expression.
class Array : public Expression {
public:
    Array(std::vector<Expression*>& array);
    ~Array();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    std::vector<std::unique_ptr<Expression>> _array;
};

typedef std::unordered_map<std::string, Expression*> KeyValueMap;
typedef std::pair<std::string, Expression*> KeyValue;

// Dictionary expression.
class Dict : public Expression {
public:
    Dict(KeyValueMap& dict);
    ~Dict();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    std::unordered_map<std::string, std::unique_ptr<Expression>> _dict;
};

// Binary expression.
class BinaryExpression : public Expression {
public:
    BinaryExpression(const std::string& op, Expression* lhs, Expression* rhs);
    ~BinaryExpression();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    std::string _op;
    std::unique_ptr<Expression> _lhs;
    std::unique_ptr<Expression> _rhs;
};

// Ternary expression.
class TernaryExpression : public Expression {
public:
    TernaryExpression(Expression* cond, Expression* lhs, Expression* rhs);
    ~TernaryExpression();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    std::unique_ptr<Expression> _cond;
    std::unique_ptr<Expression> _lhs;
    std::unique_ptr<Expression> _rhs;
};

// Logical not expression.
class NotExpression : public Expression {
public:
    NotExpression(Expression* expr);
    ~NotExpression();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    std::unique_ptr<Expression> _expr;
};

// Function call expression.
class CallExpression : public Expression {
public:
    CallExpression(const std::string& func_name, std::vector<Expression*>& args);
    ~CallExpression();
    int run (ExpressionContext& context,
            rapidjson::Value& value);
    int run(ExpressionContext& context,
            rapidjson::Value& value,
            const rapidjson::Value& input);
    int set_next(CallExpression* next);

private:
    std::string _func_name;
    std::vector<std::unique_ptr<Expression>> _args;
    std::unique_ptr<CallExpression> _next;
};

// Variable expression.
class VariableExpression : public Expression {
public:
    VariableExpression(const std::string& name);
    ~VariableExpression();
    int run(ExpressionContext& context, rapidjson::Value& value);

private:
    std::string _name;
};

}  // namespace expression
}  // namespace uskit

#endif  // USKIT_EXPRESSION_H
