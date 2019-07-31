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

#include <rapidjson/writer.h>
#include <rapidjson/pointer.h>
#include "utils.h"
#include "thread_data.h"
#include "common.h"
#include <iconv.h>

namespace uskit
{

std::string json_encode(const rapidjson::Value &json)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    return buffer.GetString();
}

int json_set_value_by_path(const std::string &path,
                           rapidjson::Document &doc,
                           rapidjson::Value &value)
{
    std::string norm_path(path);
    // Path normalization.
    if (norm_path[0] != '/')
    {
        norm_path = "/" + norm_path;
    }
    // Root path.
    if (norm_path == "/")
    {
        norm_path = "";
    }
    rapidjson::Pointer pointer(norm_path.c_str());
    pointer.Set(doc, value);

    if (pointer.IsValid())
    {
        return 0;
    }
    else
    {
        US_LOG(ERROR) << "Set value by path failed, error_code: " << pointer.GetParseErrorCode();
        return -1;
    }
}

std::string get_value_type(const rapidjson::Value &value)
{
    if (value.IsNull())
    {
        return "null";
    }
    else if (value.IsBool())
    {
        return "bool";
    }
    else if (value.IsInt())
    {
        return "int";
    }
    else if (value.IsDouble())
    {
        return "double";
    }
    else if (value.IsString())
    {
        return "string";
    }
    else if (value.IsArray())
    {
        return "array";
    }
    else if (value.IsObject())
    {
        return "obj";
    }
    else
    {
        // Never happen.
        return "unknown";
    }
}

int merge_json_objects(rapidjson::Value &to, const rapidjson::Value &from,
                       rapidjson::Document::AllocatorType &allocator)
{
    if (!to.IsObject() || !from.IsObject())
    {
        US_LOG(ERROR) << "`merge_json_objects' requires arguments to be objects";
        return -1;
    }

    for (auto &m : from.GetObject())
    {
        rapidjson::Value key(m.name, allocator);
        rapidjson::Value value(m.value, allocator);
        if (to.HasMember(key))
        {
            // Same key exists in `from` and `to`.
            // Merge if both members are objects.
            if (to[key].IsObject() && value.IsObject())
            {
                for (auto &n : value.GetObject())
                {
                    to[key].AddMember(n.name, n.value, allocator);
                }
            }
            else
            {
                // Otherwise just replace.
                to.EraseMember(key);
                to.AddMember(key, value, allocator);
            }
        }
        else
        {
            to.AddMember(key, value, allocator);
        }
    }

    return 0;
}

int ReadProtoFromTextFile(const std::string &file_name, google::protobuf::Message *proto)
{
    int fd = open(file_name.c_str(), O_RDONLY);
    if (fd == -1)
    {
        LOG(ERROR) << "Failed to open file: " << file_name;
        return -1;
    }
    google::protobuf::io::FileInputStream *input =
        new google::protobuf::io::FileInputStream(fd);
    bool success = google::protobuf::TextFormat::Parse(input, proto);
    delete input;
    close(fd);
    if (success)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int replace_all(std::string &str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    if (str.size() == 0) {
        return 0;
    }
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return 0;
}

Timer::Timer(const std::string &name) : _name(name) {}

void Timer::start()
{
    _timer.start();
}

void Timer::stop()
{
    _timer.stop();
    // Obtain thread data.
    UnifiedSchedulerThreadData *td = static_cast<UnifiedSchedulerThreadData *>(BRPC_NAMESPACE::thread_local_data());
    if (td != nullptr)
    {
        td->add_log_entry(_name, std::to_string(_timer.m_elapsed()));
    }
}

} // namespace uskit
