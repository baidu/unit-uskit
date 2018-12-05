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

%skeleton "lalr1.cc"
%require "3.0.4"
%defines

%define api.namespace {uskit::expression}
%define parser_class_name {Parser}

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires
{
#include "expression/expression.h"

namespace uskit {
namespace expression {
    class Lexer;
    class Driver;
}
}

}

%parse-param { Driver& driver }

%code {
#include "expression/driver.h"
#define yylex driver.get_next_token
}

%locations
%define parse.trace
%define parse.error verbose

%define api.token.prefix {TOKEN_}

%token END 0 "end of file";
%token LEFTPAR "(";
%token RIGHTPAR ")";
%token LEFTSQUARE "[";
%token RIGHTSQUARE "]";
%token LEFTBRACE "{";
%token RIGHTBRACE "}";
%token EQ "==";
%token NE "!=";
%token GE ">=";
%token LE "<=";
%token GT ">";
%token LT "<";
%token AND "&&";
%token OR "||";
%token BITAND "&";
%token BITOR "|";
%token PLUS "+";
%token MINUS "-";
%token MUL "*";
%token DIV "/";
%token COLON ":";
%token COMMA ",";
%token DOLLAR "$";
%token NOT "!";
%token QUESTION "?";

%token NULL "null";
%token TRUE "true";
%token FALSE "false";
%token <std::string> IDENTIFIER "identifier";
%token <std::string> STRING "string";
%token <std::string> INTEGER "integer";
%token <std::string> DOUBLE "double";

%type < Program* > program;
%type < Expression* > expr;
%type < Boolean* > boolean;
%type < Array* > array;
%type < std::vector<Expression*> > expr_list;
%type < Dict* > dict;
%type < KeyValueMap > key_value_map;
%type < KeyValue > key_value;

%start program;

%right "?";
%left "|";
%left "&";
%left "||";
%left "&&";
%left "==" "!=";
%left ">" "<" ">=" "<=";
%left "+" "-";
%left "*" "/";
%right "!" UMINUS;

%%

program:
    expr                            { $$ = new Program($1); driver.set_program($$); }
  ;

expr:
    expr "+" expr                   { $$ = new BinaryExpression("+", $1, $3); }
  | expr "-" expr                   { $$ = new BinaryExpression("-", $1, $3); }
  | expr "*" expr                   { $$ = new BinaryExpression("*",  $1, $3); }
  | expr "/" expr                   { $$ = new BinaryExpression("/", $1, $3); }
  | expr "&" expr                   { $$ = new BinaryExpression("&", $1, $3); }
  | expr "|" expr                   { $$ = new BinaryExpression("|", $1, $3); }
  | expr "&&" expr                  { $$ = new BinaryExpression("&&", $1, $3); }
  | expr "||" expr                  { $$ = new BinaryExpression("||", $1, $3); }
  | expr "==" expr                  { $$ = new BinaryExpression("==", $1, $3); }
  | expr "!=" expr                  { $$ = new BinaryExpression("!=", $1, $3); }
  | expr ">" expr                   { $$ = new BinaryExpression(">", $1, $3); }
  | expr "<" expr                   { $$ = new BinaryExpression("<", $1, $3); }
  | expr ">=" expr                  { $$ = new BinaryExpression(">=", $1, $3); }
  | expr "<=" expr                  { $$ = new BinaryExpression("<=", $1, $3); }
  | expr "?" expr ":" expr %prec "?"{ $$ = new TernaryExpression($1, $3, $5); }
  | "(" expr ")"                    { std::swap($$, $2); }
  | "!" expr %prec "!"              { $$ = new NotExpression($2); }
  | IDENTIFIER "(" expr_list ")"    { $$ = new CallExpression($1, $3); }
  | "$" IDENTIFIER                  { $$ = new VariableExpression($2); }
  | "null"                          { $$ = new Null(); }
  | "integer"                       { $$ = new Integer(std::stoi($1)); }
  | "-" "integer" %prec UMINUS      { $$ = new Integer(-std::stoi($2)); }
  | "double"                        { $$ = new Double(std::stod($1)); }
  | "-" "double" %prec UMINUS       { $$ = new Double(-std::stod($2)); }
  | "string"                        { $$ = new String($1.substr(1, $1.length() - 2)); }
  | boolean                         { $$ = $1; }
  | dict                            { $$ = $1; }
  | array                           { $$ = $1; }
  ;

boolean:
    "true"                          { $$ = new Boolean(true); }
  | "false"                         { $$ = new Boolean(false); }
  ;

array:
    "[" expr_list "]"               { $$ = new Array($2); }
  ;

expr_list:
    %empty                          { $$ = std::vector<Expression*>(); }
  | expr                            { $$ = std::vector<Expression*>(); $$.push_back($1); }
  | expr_list "," expr              { std::swap($$, $1); $$.push_back($3); }
  ;

dict:
    "{" key_value_map "}"           { $$ = new Dict($2); }
  ;

key_value_map:
    %empty                          { $$ = KeyValueMap(); }
  | key_value                       { $$ = KeyValueMap(); $$.insert($1); }
  | key_value_map "," key_value     { std::swap($$, $1); $$.insert($3); }
  ;

key_value:
    "string" ":" expr               { $$ = KeyValue($1.substr(1, $1.length() - 2), $3); }
  ;

%%

void uskit::expression::Parser::error(const location_type& l, const std::string& m) {
    driver.error(l, m);
}
