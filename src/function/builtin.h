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

#ifndef USKIT_FUNCTION_BUILTIN_H
#define USKIT_FUNCTION_BUILTIN_H

#include <vector>
#include <rapidjson/document.h>

namespace uskit {
namespace function {

// Notice: Read docs/expression.md for detailed documentation.

int parameter_check(rapidjson::Value& args, std::vector<rapidjson::Type> define_types);
// Get JSON value by Unix-like path.
int get_value_by_path(rapidjson::Value& args, rapidjson::Document& return_value);
// Set JSON value by Unix-like path.
int set_value_by_path(rapidjson::Value& args, rapidjson::Document& return_value);
// Get index of an element in an Array.
int get_index_by_key(rapidjson::Value& args, rapidjson::Document& return_value);
// Make a new array from source and assign the path with array element.
int for_each_set_by_path(rapidjson::Value& args, rapidjson::Document& return_value);
// Make a new array from source by path according to each element.
int for_each_get_by_path(rapidjson::Value& args, rapidjson::Document& return_value);
// replace value by translate dict on the path
int replace_value_by_path_and_dict(rapidjson::Value& args, rapidjson::Document& return_value);
// iterate array elements on another value-based function
int array_func(rapidjson::Value& args, rapidjson::Document& return_value);
// check whether string contain another string
int string_contain(rapidjson::Value& args, rapidjson::Document& return_value);
// get substring of args[0]
int get_sub_str(rapidjson::Value& args, rapidjson::Document& return_value);
// nomarlized element at path of array
int normalized(rapidjson::Value& args, rapidjson::Document& return_value);
// Serialize JSON object to string.
int json_encode(rapidjson::Value& args, rapidjson::Document& return_value);
// Deserialize JSON object from string.
int json_decode(rapidjson::Value& args, rapidjson::Document& return_value);
// Replace string with string
int replace_all(rapidjson::Value& args, rapidjson::Document& return_value);
// Test whether a key exists in a JSON object/array.
int has_key(rapidjson::Value& args, rapidjson::Document& return_value);
// Get array length.
int array_length(rapidjson::Value& args, rapidjson::Document& return_value);
// Get a partition of an array(from begin to end, end not included).
int array_slice(rapidjson::Value& args, rapidjson::Document& return_value);
// Convert string or bool to int value.
int int_value(rapidjson::Value& args, rapidjson::Document& return_value);
// Convert a value to bool.
// Return false if value is null/false/0/0.0/''/[]/{}.
// Otherwise return true.
int bool_value(rapidjson::Value& args, rapidjson::Document& return_value);
// Get MD5 hash of given string.
int md5_hash(rapidjson::Value& args, rapidjson::Document& return_value);
// Get SHA1 hash of given string.
int sha1_hash(rapidjson::Value& args, rapidjson::Document& return_value);
// Get Unix timestamp.
int time(rapidjson::Value& args, rapidjson::Document& return_value);
// Get hamc-sha1 result
int hmac_sha1(rapidjson::Value& args, rapidjson::Document& return_value);
// Base64 Encoder
int base64_encode(rapidjson::Value& args, rapidjson::Document& return_value);
// Random String Generator
int rand_str(rapidjson::Value& args, rapidjson::Document& return_value);
// Query Encoder
int query_encode(rapidjson::Value& args, rapidjson::Document& return_value);

} // namespace function
} // namespace uskit

#endif // USKIT_FUNCTION_BUILTIN_H
