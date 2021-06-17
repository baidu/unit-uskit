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

#include "butil.h"
#include <ctime>
#include <rapidjson/pointer.h>
#include "function/builtin.h"
#include "utils.h"
#include "common.h"
#include <set>
#include "function/function_manager.h"
#include "function/str_function.h"

namespace uskit {
namespace function {

// split str into array
int str_split(rapidjson::Value& args, rapidjson::Document& return_value) {
    std::vector<rapidjson::Type> define_types_0 = {rapidjson::kStringType, rapidjson::kStringType};

    std::vector<rapidjson::Type> define_types_1 = {rapidjson::kStringType};
    if (parameter_check(args, define_types_0) != 0 && parameter_check(args, define_types_1) != 0) {
        return -1;
    }
    std::string delimeter = " ";
    if (args.Size() == 1) {
        US_LOG(WARNING) << "delimeter is set to default[space] 'cause it is missing";
    } else {
        delimeter = args[1].GetString();
    }
    std::vector<std::string> split;
    BUTIL_NAMESPACE::SplitStringUsingSubstr(args[0].GetString(), delimeter, &split);
    return_value.SetArray();
    for (auto sp : split) {
        return_value.PushBack(
                rapidjson::Value(sp.c_str(), return_value.GetAllocator()).Move(),
                return_value.GetAllocator());
    }
    return 0;
}

// slice str to substring
int str_slice(rapidjson::Value& args, rapidjson::Document& return_value) {
    std::vector<rapidjson::Type> define_types_0 = {
            rapidjson::kStringType, rapidjson::kNumberType, rapidjson::kNumberType};
    std::vector<rapidjson::Type> define_types_1 = {rapidjson::kStringType, rapidjson::kNumberType};
    if (parameter_check(args, define_types_0) != 0 && parameter_check(args, define_types_1) != 0) {
        return -1;
    }
    std::string input = args[0].GetString();
    std::string output = "";
    if (!args[1].IsInt()) {
        US_LOG(ERROR) << "start index is not integer, " << get_value_type(args[1]);
        return -1;
    }
    int start = reverse_index(args[1].GetInt(), input.size());
    if (args.Size() == 2) {
        output = input.substr(start);
    } else if (args.Size() == 3 && args[2].IsInt()) {
        int end = reverse_index(args[2].GetInt(), input.size());
        if (start >= end || start == static_cast<int>(input.size())) {
            US_LOG(ERROR) << "start " << start << "being greater than " << end
                          << " or greater than size: " << input.size() << " is not supported";
            return -1;
        }
        output = input.substr(start, end - start);
    }
    return_value.SetString(output.c_str(), return_value.GetAllocator());

    return 0;
}

// join items to string
int array_join(rapidjson::Value& args, rapidjson::Document& return_value) {
    std::vector<rapidjson::Type> define_tye = {rapidjson::kArrayType, rapidjson::kStringType};
    if (parameter_check(args, define_tye) != 0) {
        return -1;
    }
    std::string delimeter = args[1].GetString();
    std::string output = "";
    for (auto& item : args[0].GetArray()) {
        if (item.IsString()) {
            output += item.GetString() + delimeter;
        } else {
            output += uskit::json_encode(item) + delimeter;
        }
    }
    BUTIL_NAMESPACE::TrimString(output, delimeter, &output);
    return_value.SetString(output.c_str(), return_value.GetAllocator());
    return 0;
}

// length of input str
int str_length(rapidjson::Value& args, rapidjson::Document& return_value) {
    std::vector<rapidjson::Type> define_types = {rapidjson::kStringType};
    if (parameter_check(args, define_types) != 0) {
        return -1;
    }
    std::string input = args[0].GetString();
    return_value.SetInt(input.size());
    return 0;
}

// find substr index
int str_find(rapidjson::Value& args, rapidjson::Document& return_value) {
    std::vector<rapidjson::Type> define_types_0 = {rapidjson::kStringType, rapidjson::kStringType};
    std::vector<rapidjson::Type> define_types_1 = {
            rapidjson::kStringType, rapidjson::kStringType, rapidjson::kNumberType};
    std::vector<rapidjson::Type> define_types_2 = {rapidjson::kStringType,
                                                   rapidjson::kStringType,
                                                   rapidjson::kNumberType,
                                                   rapidjson::kTrueType};
    std::vector<rapidjson::Type> define_types_3 = {rapidjson::kStringType,
                                                   rapidjson::kStringType,
                                                   rapidjson::kNumberType,
                                                   rapidjson::kFalseType};
    if (parameter_check(args, define_types_0) != 0 && parameter_check(args, define_types_1) != 0 &&
        parameter_check(args, define_types_2) != 0 && parameter_check(args, define_types_3)) {
        return -1;
    }
    std::string input = args[0].GetString();
    std::string substr = args[1].GetString();
    std::size_t index = input.npos;
    int pos = 0;
    if (args.Size() == 2) {
        index = input.find(substr);
    } else if (args.Size() >= 3) {
        pos = args[2].GetInt();
        if (args.Size() == 4 && args[3].GetBool()) {
            index = input.rfind(substr, pos);
        } else {
            index = input.find(substr, pos);
        }
    }
    if (index == input.npos)
    {
        return_value.SetInt(-1);
    } else {
        return_value.SetInt(index);
    }
    
    return 0;
}

}  // namespace function
}  // namespace uskit
