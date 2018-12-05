/*
 * Copyright (c) 2018 Baidu, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

%{
#include "expression/driver.h"

%}

%option debug
%option nodefault
%option noyywrap
%option c++
%option yyclass="uskit::expression::Lexer"

%{
#define YY_USER_ACTION _loc.columns(yyleng);
%}

%%

%{
    _loc.step(); 
%}

"null" {
    return uskit::expression::Parser::make_NULL(_loc);
}

"true" {
    return uskit::expression::Parser::make_TRUE(_loc);
}

"false" {
    return uskit::expression::Parser::make_FALSE(_loc);
}

[a-zA-Z_][a-zA-Z0-9_]* {
    return uskit::expression::Parser::make_IDENTIFIER(yytext, _loc);
}

'((\\.)|[^'\\])*' {
    return uskit::expression::Parser::make_STRING(yytext, _loc);
}

[0-9]+ {
    return uskit::expression::Parser::make_INTEGER(yytext, _loc);
}

[0-9]+\.[0-9]+ {
    return uskit::expression::Parser::make_DOUBLE(yytext, _loc);
}

"(" {
    return uskit::expression::Parser::make_LEFTPAR(_loc);
}

")" {
    return uskit::expression::Parser::make_RIGHTPAR(_loc);
}

"[" {
    return uskit::expression::Parser::make_LEFTSQUARE(_loc);
}

"]" {
    return uskit::expression::Parser::make_RIGHTSQUARE(_loc);
}

"{" {
    return uskit::expression::Parser::make_LEFTBRACE(_loc);
}

"}" {
    return uskit::expression::Parser::make_RIGHTBRACE(_loc);
}

"==" {
    return uskit::expression::Parser::make_EQ(_loc);
}

"!=" {
    return uskit::expression::Parser::make_NE(_loc);
}

">=" {
    return uskit::expression::Parser::make_GE(_loc);
}

"<=" {
    return uskit::expression::Parser::make_LE(_loc);
}

">" {
    return uskit::expression::Parser::make_GT(_loc);
}

"<" {
    return uskit::expression::Parser::make_LT(_loc);
}

"&&" {
    return uskit::expression::Parser::make_AND(_loc);
}

"||" {
    return uskit::expression::Parser::make_BITOR(_loc);
}

"&" {
    return uskit::expression::Parser::make_BITAND(_loc);
}

"|" {
    return uskit::expression::Parser::make_BITOR(_loc);
}

"+" {
    return uskit::expression::Parser::make_PLUS(_loc);
}

"-" {
    return uskit::expression::Parser::make_MINUS(_loc);
}

"*" {
    return uskit::expression::Parser::make_MUL(_loc);
}

"/" {
    return uskit::expression::Parser::make_DIV(_loc);
}

":" {
    return uskit::expression::Parser::make_COLON(_loc);
}

"," {
    return uskit::expression::Parser::make_COMMA(_loc);
}

"$" {
    return uskit::expression::Parser::make_DOLLAR(_loc);
}

"!" {
    return uskit::expression::Parser::make_NOT(_loc);
}

"?" {
    return uskit::expression::Parser::make_QUESTION(_loc);
}

[\t ] {
    /* ignore whitespace */
}

\n {
    _loc.lines();
}

. {
    driver.error(_loc, std::string("invalid character: ") + yytext);
}

<<EOF>> {
    return uskit::expression::Parser::make_END(_loc);
}

%%
