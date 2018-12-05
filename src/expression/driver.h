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

#ifndef USKIT_DRIVER_H
#define USKIT_DRIVER_H

#include <sstream>
#include "expression/lexer.h"
#include "parser.h"

namespace uskit {
namespace expression {

// Driver class for expression AST building.
class Driver {
public:
    Driver();
    virtual ~Driver();
    Parser::symbol_type get_next_token();
    // Parse expression from stream adn build AST.
    int parse(const std::string& file_name, std::istream* in);
    // Parse expression from string adn build AST.
    int parse(const std::string& file_name, const std::string& expr_str);
    // Error handling.
    void error(const location& l, const std::string& m);
    void set_program(Program* program);
    // Get parsed AST of expression.
    std::unique_ptr<Expression> get_expression();

private:
    Lexer _lexer;
    Parser _parser;

    std::string _file_name;
    std::unique_ptr<Program> _program;
};

} // namespace expression
} // namespace uskit

#endif // USKIT_DRIVER_H
