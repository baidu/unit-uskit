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

#ifndef USKIT_LEXER_H
#define USKIT_LEXER_H

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL uskit::expression::Parser::symbol_type \
    uskit::expression::Lexer::get_next_token(uskit::expression::Driver& driver)

#include "parser.h"

namespace uskit {
namespace expression {

// Forward declaration
class Driver;

// Expression lexical analyser.
class Lexer : public yyFlexLexer {

public:
    Lexer() {}
    ~Lexer() {}
    void init(std::string* file_name = nullptr) {
        _loc.initialize(file_name);
    }
    virtual Parser::symbol_type get_next_token(Driver& driver);
private:
    // Current location.
    location _loc;
};

} // namespace expression
} // namespace uskit

#endif // USKIT_LEXER_H
