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

#include <butil/strings/string_split.h>
#include "function/function_manager.h"
#include "utils.h"
#include "common.h"

namespace uskit {
namespace function {


FunctionManager::FunctionManager() {}

FunctionManager& FunctionManager::instance() {
    static FunctionManager instance;
    return instance;
}

void FunctionManager::add_function(const std::string& func_name,
                                   FunctionPtr func_ptr) {
    _function_ptr_map.emplace(func_name, func_ptr);
}

int FunctionManager::call_function(const std::string& func_name,
                                   rapidjson::Value& args,
                                   rapidjson::Document& return_value) {
    auto iter = _function_ptr_map.find(func_name);
    if (iter == _function_ptr_map.end()) {
        US_LOG(ERROR) << "Call to undefined function [" << func_name << "]";
        return -1;
    }
    FunctionPtr func_ptr = iter->second;

    // Do fucntion call
    if ((*func_ptr)(args, return_value) != 0) {
        US_LOG(ERROR) << "Call function [" << func_name << "] failed";
        return -1;
    }
    
    return 0;
}

} // namespace function
} // namespace uskit
