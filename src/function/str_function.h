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

#ifndef USKIT_FUNCTION_STR_FUNCTION_H
#define USKIT_FUNCTION_STR_FUNCTION_H

#include "rapidjson/document.h"

namespace uskit {
namespace function {

// split str into array
int str_split(rapidjson::Value& args, rapidjson::Document& return_value);
// slice str to substring
int str_slice(rapidjson::Value& args, rapidjson::Document& return_value);
// join items to string
int array_join(rapidjson::Value& args, rapidjson::Document& return_value);
// length of input str
int str_length(rapidjson::Value& args, rapidjson::Document& return_value);
// find substr index
int str_find(rapidjson::Value& args, rapidjson::Document& return_value);

}  // namespace function
}  // namespace uskit

#endif  // USKIT_FUNCTION_STR_FUNCTION_H
