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

#include <fstream>
#include "butil.h"
#include "expression/driver.h"

namespace uskit {
namespace expression {
    
Driver::Driver() : _parser(*this) {
}

Driver::~Driver() {
}

Parser::symbol_type Driver::get_next_token() {
    return _lexer.get_next_token(*this);
}

void Driver::error(const location& l, const std::string& m) {
    LOG(ERROR) << "location: " << l << ", message: " << m;
}

int Driver::parse(const std::string& file_name, std::istream* in) {
    _file_name = file_name;
    _lexer.init(&_file_name);
    _lexer.switch_streams(in, nullptr);
    return _parser.parse();
}

int Driver::parse(const std::string& file_name, const std::string& expr_str) {
    std::istringstream iss(expr_str);
    return parse(file_name, &iss);
}

void Driver::set_program(Program* program) {
    _program.reset(program);
}

std::unique_ptr<Expression> Driver::get_expression() {
    return _program->get_expression();
}

} // namespace expression
} // namespace uskit
