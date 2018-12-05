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

#include <rapidjson/document.h>

namespace uskit {
namespace function {

// Notice: Read docs/expression.md for detailed documentation.

// Set JSON value by Unix-like path.
int get_value_by_path(rapidjson::Value& args, rapidjson::Document& return_value);
// Serialize JSON object to string.
int json_encode(rapidjson::Value& args, rapidjson::Document& return_value);
// Deserialize JSON object from string.
int json_decode(rapidjson::Value& args, rapidjson::Document& return_value);
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

} // namespace function
} // namespace uskit

#endif // USKIT_FUNCTION_BUILTIN_H
