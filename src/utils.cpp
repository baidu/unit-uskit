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
#include <openssl/evp.h>

namespace uskit {

std::string json_encode(const rapidjson::Value &json) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    return buffer.GetString();
}

int json_set_value_by_path(
        const std::string &path,
        rapidjson::Document &doc,
        rapidjson::Value &value) {
    std::string norm_path(path);
    // Path normalization.
    if (norm_path[0] != '/') {
        norm_path = "/" + norm_path;
    }
    // Root path.
    if (norm_path == "/") {
        norm_path = "";
    }
    rapidjson::Pointer pointer(norm_path.c_str());
    pointer.Set(doc, value);

    if (pointer.IsValid()) {
        return 0;
    } else {
        US_LOG(ERROR) << "Set value by path failed, error_code: " << pointer.GetParseErrorCode();
        return -1;
    }
}

std::string get_value_type(const rapidjson::Value &value) {
    return TypeString[value.GetType()];
}

int merge_json_objects(
        rapidjson::Value &to,
        const rapidjson::Value &from,
        rapidjson::Document::AllocatorType &allocator) {
    if (!to.IsObject() || !from.IsObject()) {
        US_LOG(ERROR) << "`merge_json_objects' requires arguments to be objects";
        return -1;
    }

    for (auto &m : from.GetObject()) {
        rapidjson::Value key(m.name, allocator);
        rapidjson::Value value(m.value, allocator);
        if (to.HasMember(key)) {
            // Same key exists in `from` and `to`.
            // Merge if both members are objects.
            if (to[key].IsObject() && value.IsObject()) {
                /*for (auto &n : value.GetObject())
                {
                    to[key].AddMember(n.name, n.value, allocator);
                }*/
                merge_json_objects(to[key], value, allocator);
            } else {
                // Otherwise just replace.
                to.EraseMember(key);
                to.AddMember(key, value, allocator);
            }
        } else {
            to.AddMember(key, value, allocator);
        }
    }

    return 0;
}

int ReadProtoFromTextFile(const std::string &file_name, google::protobuf::Message *proto) {
    int fd = open(file_name.c_str(), O_RDONLY);
    if (fd == -1) {
        LOG(ERROR) << "Failed to open file: " << file_name;
        return -1;
    }
    google::protobuf::io::FileInputStream *input = new google::protobuf::io::FileInputStream(fd);
    bool success = google::protobuf::TextFormat::Parse(input, proto);
    delete input;
    close(fd);
    if (success) {
        return 0;
    } else {
        return -1;
    }
}

int replace_all(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    if (str.size() == 0) {
        return 0;
    }
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // Handles case where 'to' is a substring of 'from'
    }
    return 0;
}

Timer::Timer(const std::string &name) : _name(name) {}

void Timer::start() {
    _timer.start();
}

void Timer::stop() {
    _timer.stop();
    // Obtain thread data.
    UnifiedSchedulerThreadData *td =
            static_cast<UnifiedSchedulerThreadData *>(BRPC_NAMESPACE::thread_local_data());
    if (td != nullptr) {
        td->add_log_entry(_name, std::to_string(_timer.m_elapsed()));
    }
}

int reverse_index(int r_index, int size) {
    int index = r_index;
    if (index < 0) {
        index += size;
    }
    if (index < 0) {
        index = 0;
    }
    if (index > size) {
        index = size;
    }
    return index;
}

std::string digest_from_char(const void* data, size_t length, const std::string& type) {
    const EVP_MD* md = nullptr;
    if (type == "md5") {
        md = EVP_md5();
    } else if (type == "sha1") {
        md = EVP_sha1();
    } else {
        US_LOG(ERROR) << "invalid digest type: " << type;
    }
    unsigned char digest[EVP_MAX_MD_SIZE] = {'\0'};
    unsigned int digest_len = 0;
    EVP_Digest(data, length, digest, &digest_len, md, NULL);
    std::string digest_str = digest_to_base16(digest, digest_len);
    return digest_str;
}

std::string digest_to_base16(const unsigned char* data, size_t length) {
    std::string out_str;
    const char map[] = "0123456789abcdef";
    for (size_t i = 0; i < length; ++i) {
        out_str += map[data[i] / 16];
        out_str += map[data[i] % 16];
    }
    return out_str;
}

void fast_rand_bytes(void* output, size_t output_length) {
    const size_t n = output_length / 8;
    for (size_t i = 0; i < n; ++i) {
        static_cast<uint64_t*>(output)[i] = BUTIL_NAMESPACE::fast_rand();
    }
    const size_t m = output_length - n * 8;
    if (m) {
        uint8_t* p = static_cast<uint8_t*>(output) + n * 8;
        uint64_t r = BUTIL_NAMESPACE::fast_rand();
        for (size_t i = 0; i < m; ++i) {
            p[i] = (r & 0xFF);
            r = (r >> 8);
        }
    }
}

}  // namespace uskit
