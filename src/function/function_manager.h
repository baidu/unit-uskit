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

#ifndef USKIT_FUNCTION_FUNCTION_MANAGER_H
#define USKIT_FUNCTION_FUNCTION_MANAGER_H

#include <string>
#include <unordered_map>
#include <rapidjson/document.h>

namespace uskit {
namespace function {

// Function pointer prototype.
typedef int (*FunctionPtr)(rapidjson::Value& args,
                           rapidjson::Document& return_value);

// Class for managing global functions.
class FunctionManager {
public:
    FunctionManager(const FunctionManager&) = delete;
    FunctionManager& operator=(const FunctionManager&) = delete;

    // Singleton
    static FunctionManager& instance();

    // Add function with specified name.
    void add_function(const std::string& func_name,
                      FunctionPtr func_ptr);

    // Call function with specified name and arguments.
    int call_function(const std::string& func_name,
                      rapidjson::Value& args,
                      rapidjson::Document& return_value);
private:
    FunctionManager();
    std::unordered_map<std::string, FunctionPtr> _function_ptr_map;
};

} // namespace function

// Macro for global function registering.
#define REGISTER_FUNCTION(func_name, func_ptr) \
    function::FunctionManager::instance().add_function(func_name, func_ptr)

} // namespace uskit

#endif // USKIT_FUNCTION_FUNCTION_MANAGER_H
