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

namespace uskit {
namespace function {

int get_value_by_path(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 2 && args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 2 or 3 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsObject() && !args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be object or array, "
                      << get_value_type(args[0]) << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }

    std::string path = args[1].GetString();
    // Path normalization.
    if (path[0] != '/') {
        path = "/" + path;
    }
    // Root path.
    if (path == "/") {
        path = "";
    }
    US_DLOG(INFO) << "Get Path: " << path;
    rapidjson::Pointer pointer(path.c_str());
    rapidjson::Value* value = rapidjson::GetValueByPointer(args[0], pointer);
    if (value == nullptr) {
        US_DLOG(INFO) << "value is null";
        if (args.Size() == 2) {
            return_value.SetNull();
        } else {
            rapidjson::Document::AllocatorType& return_alloc = return_value.GetAllocator();
            return_value.CopyFrom(args[2], return_alloc);
        }
    } else {
        US_DLOG(INFO) << "value not null, start copy";
        rapidjson::Document::AllocatorType& return_alloc = return_value.GetAllocator();
        return_value.CopyFrom(*value, return_alloc);
    }

    return 0;
}

int for_each_set_by_path(rapidjson::Value& args, rapidjson::Document& return_value) {
    /*
    args[0]: list
    args[1]: path
    */
    if (args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 2 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be object, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    if (!args[2].IsArray() || args[2].GetArray().Size() != args[0].GetArray().Size()) {
        US_LOG(ERROR) << "Third argument's length must equal to the first when they are all array. "
                      << "len(args[0]): " << args[0].GetArray().Size()
                      << ", len(args[2]): " << args[2].GetArray().Size();
        return -1;
    }
    std::string path = args[1].GetString();
    // Path normalization.
    if (path[0] != '/') {
        path = "/" + path;
    }
    // Root path.
    if (path == "/") {
        path = "";
    }
    return_value.SetArray();
    int index = 0;
    for (auto& iter : args[0].GetArray()) {
        rapidjson::Document d;
        d.CopyFrom(iter, d.GetAllocator());
        if (args[2].IsArray()) {
            uskit::json_set_value_by_path(path, d, args[2][index++]);
        } else {
            uskit::json_set_value_by_path(path, d, args[2]);
        }
        rapidjson::Value ele_value(d, return_value.GetAllocator());
        return_value.PushBack(ele_value, return_value.GetAllocator());
    }

    return 0;
}  // namespace function

int set_value_by_path(rapidjson::Value& args, rapidjson::Document& return_value) {
    /*
    args[0]: list
    args[1]: path
    args[2]: value
    */
    if (args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 3 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsObject() && !args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be object or array, "
                      << get_value_type(args[0]) << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    rapidjson::Value copyvalue(args[0], return_value.GetAllocator());
    std::string path = args[1].GetString();
    // Path normalization.
    if (path[0] != '/') {
        path = "/" + path;
    }
    // Root path.
    if (path == "/") {
        path = "";
    }
    return_value.CopyFrom(copyvalue, return_value.GetAllocator());
    if (uskit::json_set_value_by_path(path, return_value, args[2]) != 0) {
        US_LOG(ERROR) << "set value by path error";
        return -1;
    }
    return 0;
}

int replace_value_by_path_and_dict(rapidjson::Value& args, rapidjson::Document& return_value) {
    /*
    args[0]: list
    args[1]: path
    args[2]: dict
    */
    if (args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 3 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be object, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    if (!args[2].IsObject()) {
        US_LOG(ERROR) << "Function expects third argument to be dict, " << get_value_type(args[2])
                      << " were given";
        return -1;
    }
    std::string path = args[1].GetString();
    // Path normalization.
    if (path[0] != '/') {
        path = "/" + path;
    }
    // Root path.
    if (path == "/") {
        path = "";
    }
    return_value.SetArray();
    for (auto& iter : args[0].GetArray()) {
        rapidjson::Value* value =
                rapidjson::GetValueByPointer(iter, rapidjson::Pointer(path.c_str()));
        if (value == nullptr || !value->IsString()) {
            rapidjson::Value ele_value(iter, return_value.GetAllocator());
            return_value.PushBack(ele_value, return_value.GetAllocator());
            continue;
        }
        std::string key = value->GetString();
        rapidjson::Value::MemberIterator val_itr = args[2].FindMember(key.c_str());
        if (val_itr != args[2].MemberEnd()) {
            std::string replace_value = val_itr->value.GetString();
            value->SetString(
                    replace_value.c_str(), replace_value.size(), return_value.GetAllocator());
        }
        rapidjson::Value ele_value(iter, return_value.GetAllocator());
        return_value.PushBack(ele_value, return_value.GetAllocator());
    }

    return 0;
}

int string_contain(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 2) {
        US_LOG(ERROR) << "Function expects 2 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[0]) << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    std::string source = args[0].GetString();
    std::string target = args[1].GetString();
    return_value.SetBool(source.find(target) != source.npos);
    return 0;
}

int get_sub_str(rapidjson::Value& args, rapidjson::Document& return_value) {
    /*
    args[0]: source string
    args[1]: string
    args[2]: if contain args[1]
    args[3]: if reverse
    */
    if (args.Size() != 2 && args.Size() != 3 && args.Size() != 4) {
        US_LOG(ERROR) << "Function expects 2~4 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects first argument to be string, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    bool is_contain = false;
    bool is_reverse = false;
    if (args.Size() >= 3) {
        if (!args[2].IsBool()) {
            US_LOG(ERROR) << "Function expects third argument to be boolean, "
                          << get_value_type(args[2]) << " were given";
            return -1;
        }
        is_contain = args[2].GetBool();
    }
    if (args.Size() == 4) {
        if (!args[3].IsBool()) {
            US_LOG(ERROR) << "Function expects fourth argument to be boolean, "
                          << get_value_type(args[3]) << " were given";
            return -1;
        }
        is_reverse = args[3].GetBool();
    }
    std::string source = args[0].GetString();
    std::string target = args[1].GetString();
    int pos = is_reverse ? source.rfind(target) : source.find(target);
    if (pos != std::string::npos) {
        std::string substr = is_contain ? source.substr(pos) : source.substr(pos + target.size());
        return_value.SetString(substr.c_str(), substr.length(), return_value.GetAllocator());
        return 0;
    }
    return_value.SetString(source.c_str(), source.size(), return_value.GetAllocator());
    return 0;
}

int array_func(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 2) {
        US_LOG(ERROR) << "Function expects at least 3 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be array, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    if (!args[1].IsArray()) {
        US_LOG(ERROR) << "Function expects second argument to be array, " << get_value_type(args[1])
                      << " were given";
        return -1;
    }
    return_value.SetArray();
    for (size_t array_index = 0; array_index != args[0].Size(); ++array_index) {
        rapidjson::Value value(args[0][array_index], return_value.GetAllocator());
        rapidjson::Value args_array(rapidjson::kArrayType);
        rapidjson::Value ele_value(value, return_value.GetAllocator());
        args_array.PushBack(ele_value, return_value.GetAllocator());
        if (args[1].Size() != 2) {
            US_LOG(ERROR) << "Function expects third argument to be array with 2 elements, "
                          << args[2].Size() << " were given";
            return -1;
        }
        if (!args[1][0].IsString()) {
            US_LOG(ERROR) << "Function expects the first element in third argument to be string, "
                          << get_value_type(args[1][0]) << " were given";
            return -1;
        }
        std::string func_name = args[1][0].GetString();
        if (!args[1][1].IsArray()) {
            US_LOG(ERROR) << "Function expects the second element in third argument to be array, "
                          << get_value_type(args[1][1]) << " were given";
            return -1;
        }
        for (auto& iter : args[1][1].GetArray()) {
            rapidjson::Value iter_value(iter, return_value.GetAllocator());
            args_array.PushBack(iter_value, return_value.GetAllocator());
        }
        rapidjson::Document internal_value(&return_value.GetAllocator());
        if (function::FunctionManager::instance().call_function(
                    func_name, args_array, internal_value) != 0) {
            US_LOG(ERROR) << "Call func " << func_name << "Failed";
            return -1;
        }
        value.Swap(internal_value);
        rapidjson::Value element_value(value, return_value.GetAllocator());
        return_value.PushBack(element_value, return_value.GetAllocator());
    }
    return 0;
}

int normalized(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() <= 3) {
        US_LOG(ERROR) << "Function expects at least 3 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be array, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    if (!args[2].IsArray()) {
        US_LOG(ERROR) << "Function expects third argument to be array, " << get_value_type(args[2])
                      << " were given";
        return -1;
    }
    std::string path = args[1].GetString();
    // Path normalization.
    if (path[0] != '/') {
        path = "/" + path;
    }
    // Root path.
    if (path == "/") {
        path = "";
    }
    return_value.SetArray();
    for (size_t array_index = 0; array_index != args[0].Size(); ++array_index) {
        rapidjson::Value* value = rapidjson::GetValueByPointer(
                args[0][array_index], rapidjson::Pointer(path.c_str()));
        if (value == nullptr || !value->IsString()) {
            US_LOG(ERROR) << "value at path: " << path << "not found";
            return -1;
        }
        rapidjson::Value args_array(rapidjson::kArrayType);
        rapidjson::Value ele_value(*value, return_value.GetAllocator());
        args_array.PushBack(ele_value, return_value.GetAllocator());
        if (args[2].Size() != 2) {
            US_LOG(ERROR) << "Function expects third argument to be array with 2 elements, "
                          << args[2].Size() << " were given";
            return -1;
        }
        if (!args[2][0].IsString()) {
            US_LOG(ERROR) << "Function expects the first element in third argument to be string, "
                          << get_value_type(args[2][0]) << " were given";
            return -1;
        }
        std::string func_name = args[2][0].GetString();
        if (!args[2][1].IsArray()) {
            US_LOG(ERROR) << "Function expects the second element in third argument to be array, "
                          << get_value_type(args[2][1]) << " were given";
            return -1;
        }
        for (auto& iter : args[2][1].GetArray()) {
            rapidjson::Value iter_value(iter, return_value.GetAllocator());
            args_array.PushBack(iter_value, return_value.GetAllocator());
        }
        rapidjson::Document internal_value(&return_value.GetAllocator());
        if (function::FunctionManager::instance().call_function(
                    func_name, args_array, internal_value) != 0) {
            US_LOG(ERROR) << "Call func " << func_name << "Failed";
            return -1;
        }
        rapidjson::Value contain_key(false);
        contain_key.Swap(internal_value);
        if (!contain_key.IsBool()) {
            US_LOG(ERROR) << "Result of func [" << func_name << "] is "
                          << get_value_type(contain_key) << "other than boolean";
            return -1;
        }
        if (contain_key.GetBool()) {
            std::string func_name = args[3][0].GetString();
            if (!args[3][1].IsArray()) {
                US_LOG(ERROR)
                        << "Function expects the second element in third argument to be array, "
                        << get_value_type(args[3][1]) << " were given";
                return -1;
            }
            rapidjson::Value args_array(rapidjson::kArrayType);
            rapidjson::Value ele_value(*value, return_value.GetAllocator());
            args_array.PushBack(ele_value, return_value.GetAllocator());
            for (auto& iter : args[3][1].GetArray()) {
                rapidjson::Value iter_value(iter, return_value.GetAllocator());
                args_array.PushBack(iter_value, return_value.GetAllocator());
            }
            rapidjson::Document internal_value(&return_value.GetAllocator());
            if (function::FunctionManager::instance().call_function(
                        func_name, args_array, internal_value) != 0) {
                US_LOG(ERROR) << "Call func " << func_name << "Failed";
                return -1;
            }
            value->Swap(internal_value);
            if (!value->IsString()) {
                US_LOG(ERROR) << "Result of func [" << func_name << "] is "
                              << get_value_type(*value) << "other than string";
                return -1;
            }
        }
        rapidjson::Value element_value(args[0][array_index], return_value.GetAllocator());
        return_value.PushBack(element_value, return_value.GetAllocator());
    }
    return 0;
}

int get_index_by_key(rapidjson::Value& args, rapidjson::Document& return_value) {
    /*
    args[0]: list
    args[1]: path
    args[2]: key
    return_value: index of bot_id in response_list
    */
    if (args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 3 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be object, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    if (!args[2].IsString()) {
        US_LOG(ERROR) << "Function expects third argument to be string, " << get_value_type(args[2])
                      << " were given";
        return -1;
    }
    std::string path = args[1].GetString();
    // Path normalization.
    if (path[0] != '/') {
        path = "/" + path;
    }
    // Root path.
    if (path == "/") {
        path = "";
    }
    std::string index_str = "-1";
    return_value.SetString(index_str.c_str(), index_str.length(), return_value.GetAllocator());
    for (size_t index = 0; index != args[0].GetArray().Size(); ++index) {
        rapidjson::Value* value =
                rapidjson::GetValueByPointer(args[0][index], rapidjson::Pointer(path.c_str()));
        if (std::string(value->GetString()) == std::string(args[2].GetString())) {
            index_str = std::to_string(index);
            return_value.SetString(
                    index_str.c_str(), index_str.length(), return_value.GetAllocator());
            break;
        }
    }
    return 0;
}

int for_each_get_by_path(rapidjson::Value& args, rapidjson::Document& return_value) {
    /*
    args[0]: list
    args[1]: path
    */
    if (args.Size() != 2 && args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 2 or 3 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsArray()) {
        US_LOG(ERROR) << "Function expects first argument to be object, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    if (!args[1].IsString()) {
        US_LOG(ERROR) << "Function expects second argument to be string, "
                      << get_value_type(args[1]) << " were given";
        return -1;
    }
    std::string path = args[1].GetString();
    // Path normalization.
    if (path[0] != '/') {
        path = "/" + path;
    }
    // Root path.
    if (path == "/") {
        path = "";
    }
    return_value.SetArray();
    for (auto& iter : args[0].GetArray()) {
        rapidjson::Value* value =
                rapidjson::GetValueByPointer(iter, rapidjson::Pointer(path.c_str()));
        if (value != nullptr) {
            rapidjson::Value ele_value(*value, return_value.GetAllocator());
            return_value.PushBack(ele_value, return_value.GetAllocator());
        } else {
            rapidjson::Value ele_value;
            if (args.Size() == 2) {
                ele_value.SetNull();
            } else {
                ele_value.CopyFrom(args[2], return_value.GetAllocator());
            }
            return_value.PushBack(ele_value, return_value.GetAllocator());
        }
    }
    return 0;
}

int json_encode(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, " << args.Size() << " were given";
        return -1;
    }
    std::string json_str = uskit::json_encode(args[0]);
    return_value.SetString(json_str.c_str(), json_str.length(), return_value.GetAllocator());
    return 0;
}

int json_decode(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects first argument to be string, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }
    std::string json_str = args[0].GetString();
    US_DLOG(INFO) << "parse josn: " << json_str;
    if (return_value.Parse(json_str.c_str()).HasParseError()) {
        US_LOG(ERROR) << "Failed to parse json: " << json_str;
        return -1;
    }

    return 0;
}

int replace_all(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 3 arguments, " << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsString() || !args[1].IsString()) {
        US_LOG(ERROR) << "Function expects all arguments to be string, " << get_value_type(args[0])
                      << ", " << get_value_type(args[1]) << " and " << get_value_type(args[2])
                      << " were given";
        return -1;
    }
    std::string source = args[0].GetString();
    if (uskit::replace_all(source, args[1].GetString(), args[2].GetString()) != 0) {
        US_LOG(ERROR) << "String replace all failed";
        return -1;
    }
    return_value.SetString(source.c_str(), source.length(), return_value.GetAllocator());
    return 0;
}

int has_key(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 2) {
        US_LOG(ERROR) << "Function expects 2 argument, " << args.Size() << " were given";
        return -1;
    }

    if (args[0].IsObject()) {
        if (!args[1].IsString()) {
            return -1;
        }
        return_value.SetBool(args[0].HasMember(args[1].GetString()));
        return 0;
    }

    // O(n) for array checking.
    if (args[0].IsArray()) {
        return_value.SetBool(false);
        for (auto& v : args[0].GetArray()) {
            if (v == args[1]) {
                return_value.SetBool(true);
            }
        }
        return 0;
    }

    return -1;
}

int array_length(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, " << args.Size() << " were given";
        return -1;
    }

    if (!args[0].IsArray()) {
        return -1;
    }

    int len = args[0].Size();
    return_value.SetInt(len);

    return 0;
}

int array_slice(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (!args[0].IsArray()) {
        return -1;
    }

    int start = 0;
    int end = args[0].Size();
    int size = args[0].Size();
    if (args.Size() >= 2) {
        if (args[1].IsArray()) {
            // slice array by remove elements at index in args[1]
            std::set<int> index_to_mv;
            return_value.SetArray();
            for (auto& index : args[1].GetArray()) {
                index_to_mv.insert(index.GetInt());
            }
            for (size_t index = 0; index != args[0].Size(); ++index) {
                if (index_to_mv.find(index) == index_to_mv.end()) {
                    rapidjson::Value value(args[0][index], return_value.GetAllocator());
                    return_value.PushBack(value, return_value.GetAllocator());
                }
            }
            return 0;
        }
        if (args[1].IsObject()) {
            rapidjson::Value::ConstMemberIterator iter = args[1].FindMember("method");
            if (iter == args[1].MemberEnd()) {
                US_LOG(ERROR) << "value of key: [method] should be provided";
                return -1;
            }
            if (!iter->value.IsString()) {
                US_LOG(ERROR) << "value of key: [method] should be string, "
                              << get_value_type(iter->value) << " were provided";
                return -1;
            }
            std::string method = iter->value.GetString();
            iter = args[1].FindMember("path");
            if (iter == args[1].MemberEnd()) {
                US_LOG(ERROR) << "value of key: [path] should be provided";
                return -1;
            }
            if (!iter->value.IsString()) {
                US_LOG(ERROR) << "value of key: [path] should be string, "
                              << get_value_type(iter->value) << " were provided";
                return -1;
            }
            std::string path = iter->value.GetString();
            // Path normalization.
            if (path[0] != '/') {
                path = "/" + path;
            }
            // Root path.
            if (path == "/") {
                path = "";
            }
            iter = args[1].FindMember("keys");
            if (iter == args[1].MemberEnd()) {
                US_LOG(ERROR) << "value of key: [keys] should be provided";
                return -1;
            }
            if (!iter->value.IsArray()) {
                US_LOG(ERROR) << "value of key: [keys] should be array, "
                              << get_value_type(iter->value) << " were provided";
                return -1;
            }
            std::set<std::string> keys_to_mv;
            return_value.SetArray();
            for (auto& val : iter->value.GetArray()) {
                US_LOG(INFO) << "Inserting :" << uskit::json_encode(val);
                std::string v = val.IsString() ? val.GetString() : uskit::json_encode(val);
                keys_to_mv.insert(v);
            }
            for (auto& iter : args[0].GetArray()) {
                rapidjson::Value* value =
                        rapidjson::GetValueByPointer(iter, rapidjson::Pointer(path.c_str()));
                bool filtered = value == nullptr;
                if (method == std::string("AND")) {
                    if (value != nullptr) {
                        std::string check =
                                value->IsString() ? value->GetString() : uskit::json_encode(*value);
                        if (keys_to_mv.find(check) == keys_to_mv.end()) {
                            filtered = true;
                        }
                    }
                } else if (method == std::string("EXCLUSIVE")) {
                    if (value != nullptr) {
                        std::string check =
                                value->IsString() ? value->GetString() : uskit::json_encode(*value);
                        US_LOG(INFO) << "check value: " << check;
                        if (keys_to_mv.find(check) != keys_to_mv.end()) {
                            filtered = true;
                        } else {
                            US_LOG(INFO) << "value [" << check << "] not in keys_to_mv";
                            US_LOG(INFO) << "is user_poi in keys_to_mv: "
                                         << (keys_to_mv.find("user_poi") != keys_to_mv.end());
                        }
                    }
                }
                if (!filtered) {
                    rapidjson::Value value(iter, return_value.GetAllocator());
                    return_value.PushBack(value, return_value.GetAllocator());
                }
            }
            return 0;
        }
        if (!args[1].IsInt()) {
            return -1;
        }
        start = args[1].GetInt();
        if (start < 0) {
            start += size;
        }
        if (start < 0) {
            start = 0;
        }
        if (start > size) {
            start = size;
        }
    }

    if (args.Size() >= 3) {
        if (!args[2].IsInt()) {
            return -1;
        }
        end = args[2].GetInt();
        if (end < 0) {
            end += size;
        }
        if (end < 0) {
            end = 0;
        }
        if (end > size) {
            end = size;
        }
    }

    return_value.SetArray();
    while (start < end) {
        rapidjson::Value value(args[0][start++], return_value.GetAllocator());
        return_value.PushBack(value, return_value.GetAllocator());
    }

    return 0;
}

int int_value(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, " << args.Size() << " were given";
        return -1;
    }

    if (args[0].IsBool()) {
        if (args[0].GetBool()) {
            return_value.SetInt(1);
        } else {
            return_value.SetInt(0);
        }
    }

    if (args[0].IsString()) {
        std::string int_str = args[0].GetString();
        // Try converting from string to int.
        try {
            int int_val = std::stoi(int_str);
            return_value.SetInt(int_val);
        } catch (const std::invalid_argument& e) {
            US_LOG(ERROR) << "Failed to convert string to int, invalid argument: " << int_str;
            return -1;
        } catch (const std::out_of_range& e) {
            // Out of range
            US_LOG(ERROR) << "Failed to convert string to int, out of range: " << int_str;
            return -1;
        }
    }

    return 0;
}

int bool_value(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, " << args.Size() << " were given";
        return -1;
    }

    if (args[0].IsBool()) {
        return_value.SetBool(args[0].GetBool());
    } else if (args[0].IsNull()) {
        return_value.SetBool(false);
    } else if (args[0].IsNumber() && args[0].GetDouble() == 0) {
        return_value.SetBool(false);
    } else if (args[0].IsString()) {
        std::string str = args[0].GetString();
        if (str.empty()) {
            return_value.SetBool(false);
        } else {
            return_value.SetBool(true);
        }
    } else if (args[0].IsArray() && args[0].Empty()) {
        return_value.SetBool(false);
    } else if (args[0].IsObject() && args[0].ObjectEmpty()) {
        return_value.SetBool(false);
    } else {
        return_value.SetBool(true);
    }

    return 0;
}

int md5_hash(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, " << args.Size() << " were given";
        return -1;
    }

    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects first argument to be string, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }

    std::string raw_str = args[0].GetString();
    std::string md5_str = BUTIL_NAMESPACE::MD5String(raw_str);
    return_value.SetString(md5_str.c_str(), md5_str.length(), return_value.GetAllocator());

    return 0;
}

std::string sha1_to_base16(const std::string& sha1) {
    static char const encode[] = "0123456789abcdef";

    std::string ret;
    ret.resize(sha1.size() * 2);

    int j = 0;
    for (size_t i = 0; i < sha1.size(); ++i) {
        int a = sha1[i];
        ret[j++] = encode[(a >> 4) & 0xf];
        ret[j++] = encode[a & 0xf];
    }

    return ret;
}

int sha1_hash(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, " << args.Size() << " were given";
        return -1;
    }

    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects first argument to be string, " << get_value_type(args[0])
                      << " were given";
        return -1;
    }

    std::string raw_str = args[0].GetString();
    std::string sha1_str = BUTIL_NAMESPACE::SHA1HashString(raw_str);
    std::string sha1_base16_str = sha1_to_base16(sha1_str);
    return_value.SetString(
            sha1_base16_str.c_str(), sha1_base16_str.length(), return_value.GetAllocator());

    return 0;
}

int time(rapidjson::Value& args, rapidjson::Document& return_value) {
    std::time_t timestamp = std::time(nullptr);
    if (args.Size() == 1) {
        if (!args[0].IsString()) {
            US_LOG(ERROR) << "Function expects seconde argement to be string, "
                          << get_value_type(args[0]) << "were given";
            return -1;
        }
        std::tm now_tm ;
        localtime_r(&timestamp, &now_tm);
        char buffer[128];
        std::strftime(buffer, sizeof(buffer), args[0].GetString(), &now_tm);
        std::string ret_str(buffer);
        return_value.SetString(ret_str.c_str(), ret_str.length(), return_value.GetAllocator());
        return 0;
    }
    return_value.SetInt(timestamp);
    return 0;
}

}  // namespace function
}  // namespace uskit
