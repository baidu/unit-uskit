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

#ifndef USKIT_UTILS_H
#define USKIT_UTILS_H

#include <string>
#include <rapidjson/document.h>

#include <fcntl.h>
#include <unistd.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "butil.h"
#include "common.h"

namespace uskit {

// Serialize JSON object to string.
std::string json_encode(const rapidjson::Value& json);

// Set JSON value by Unix-like path, reference: http://rapidjson.org/md_doc_pointer.html
// Returns 0 on success, -1 otherwise.
int json_set_value_by_path(
        const std::string& path,
        rapidjson::Document& doc,
        rapidjson::Value& value);

// Get type of JSON value, including null, bool, int, double, string, array and object.
std::string get_value_type(const rapidjson::Value& value);

// Merge JSON object `from' into `to', only merge object of same key at first level.
// Example:
// from : {"a": {"b": 1}, "c": 2}
// to : {"a": {"d" : 2}, "c": [1, 2]}
// merged : {"a": {"b": 1, "d": 2}, "c": 2}
// Returns 0 on success, -1 otherwise.
int merge_json_objects(
        rapidjson::Value& to,
        const rapidjson::Value& from,
        rapidjson::Document::AllocatorType& allocator);

// Read Protobuf text format.
// Returns 0 on success, -1 otherwise.
int ReadProtoFromTextFile(const std::string& file_name, google::protobuf::Message* proto);

// ReplaceAll
int replace_all(std::string &str, const std::string& from, const std::string& to);

// Wrapper for BUTIL_NAMESPACE::Timer.
class Timer {
public:
    Timer(const std::string& name);
    // Start the timer.
    void start();
    // Stop the timer and add the elapsed time to thread data for logging.
    void stop();

private:
    std::string _name;
    // Underlying timer.
    BUTIL_NAMESPACE::Timer _timer;
};



}  // namespace uskit

#endif  // USKIT_UTILS_H
