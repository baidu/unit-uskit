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

#include <butil/md5.h>
#include <butil/sha1.h>
#include <ctime>
#include <rapidjson/pointer.h>
#include "function/builtin.h"
#include "utils.h"
#include "common.h"

namespace uskit {
namespace function {

int get_value_by_path(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 2 && args.Size() != 3) {
        US_LOG(ERROR) << "Function expects 2 or 3 argument, "
            << args.Size() << " were given";
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
    rapidjson::Pointer pointer(path.c_str());
    rapidjson::Value* value = rapidjson::GetValueByPointer(args[0], pointer);
    if (value == nullptr) {
        if (args.Size() == 2) {
            return_value.SetNull();
        } else {
            return_value.CopyFrom(args[2], return_value.GetAllocator());
        }
    } else {
        return_value.CopyFrom(*value, return_value.GetAllocator());
    }

    return 0;
}

int json_encode(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, "
            << args.Size() << " were given";
        return -1;
    }
    std::string json_str = uskit::json_encode(args[0]);
    return_value.SetString(json_str.c_str(), json_str.length(), return_value.GetAllocator());
    return 0;
}

int json_decode(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 1) {
        US_LOG(ERROR) << "Function expects 1 argument, "
            << args.Size() << " were given";
        return -1;
    }
    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects first argument to be string, "
            << get_value_type(args[0]) << " were given";
        return -1;
    }
    std::string json_str = args[0].GetString();
    if (return_value.Parse(json_str.c_str()).HasParseError()) {
        US_LOG(ERROR) << "Failed to parse json: " << json_str;
        return -1;
    }

    return 0;
}

int has_key(rapidjson::Value& args, rapidjson::Document& return_value) {
    if (args.Size() != 2) {
        US_LOG(ERROR) << "Function expects 2 argument, "
            << args.Size() << " were given";
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
        for (auto & v : args[0].GetArray()) {
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
        US_LOG(ERROR) << "Function expects 1 argument, "
            << args.Size() << " were given";
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
        US_LOG(ERROR) << "Function expects 1 argument, "
            << args.Size() << " were given";
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
        US_LOG(ERROR) << "Function expects 1 argument, "
            << args.Size() << " were given";
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
        US_LOG(ERROR) << "Function expects 1 argument, "
            << args.Size() << " were given";
        return -1;
    }

    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects first argument to be string, "
            << get_value_type(args[0]) << " were given";
        return -1;
    }

    std::string raw_str = args[0].GetString();
    std::string md5_str = butil::MD5String(raw_str);
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
        US_LOG(ERROR) << "Function expects 1 argument, "
            << args.Size() << " were given";
        return -1;
    }

    if (!args[0].IsString()) {
        US_LOG(ERROR) << "Function expects first argument to be string, "
            << get_value_type(args[0]) << " were given";
        return -1;
    }

    std::string raw_str = args[0].GetString();
    std::string sha1_str = butil::SHA1HashString(raw_str);
    std::string sha1_base16_str = sha1_to_base16(sha1_str);
    return_value.SetString(sha1_base16_str.c_str(),
            sha1_base16_str.length(), return_value.GetAllocator());

    return 0;

}

int time(rapidjson::Value& args, rapidjson::Document& return_value) {
    std::time_t timestamp = std::time(nullptr);
    return_value.SetInt(timestamp);
    return 0;
}

} // namespace function
} // namespace uskit
