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

#include <brpc/redis.h>
#include "policy/backend/redis_policy.h"
#include "utils.h"

namespace uskit {
namespace policy {
namespace backend {

int RedisRequestPolicy::init(const RequestConfig& config, const Backend* backend) {
    _backend = backend;
    const BackendRequestConfig* template_config = nullptr;
    if (config.has_include()) {
        template_config = backend->request_config(config.include());
    }
    if (_request_config.init(config, template_config) != 0) {
        LOG(ERROR) << "Failed to init redis request config";
        return -1;
    }

    return 0;
}

int RedisRequestPolicy::run(BackendController* cntl) const {
    RedisController* redis_cntl = static_cast<RedisController*>(cntl);
    brpc::Controller& brpc_cntl = redis_cntl->brpc_controller();
    expression::ExpressionContext request_context("redis request block", redis_cntl->context());

    // Generate request config dynamically.
    if (_request_config.run(request_context) != 0) {
        US_LOG(ERROR) << "Failed to generate redis request config";
        return -1;
    }
    US_DLOG(INFO) << "Generated request config: " << request_context.str();

    rapidjson::Value* redis_cmd = request_context.get_variable("redis_cmd");
    if (redis_cmd == nullptr) {
        US_LOG(ERROR) << "Required redis command";
        return -1;
    } else {
        butil::StringPiece components[64];
        brpc::RedisRequest redis_request;
        // Add Redis commands.
        for (auto & cmd : redis_cmd->GetArray()) {
            components[0] = cmd["op"].GetString();
            int size = 1;
            for (auto & arg : cmd["arg"].GetArray()) {
                components[size++] = arg.GetString();
            }
            redis_request.AddCommandByComponents(components, size);
        }

        brpc::Channel* channel = _backend->channel();
        brpc::RedisResponse& redis_response = redis_cntl->redis_response();
        channel->CallMethod(nullptr, &brpc_cntl, &redis_request, &redis_response, brpc::DoNothing());
    }

    return 0;
}

int RedisResponsePolicy::init(const ResponseConfig& config, const Backend* backend) {
    const BackendResponseConfig* template_config = nullptr;
    if (config.has_include()) {
        template_config = backend->response_config(config.include());
    }
    if (_response_config.init(config, template_config) != 0) {
        LOG(ERROR) << "Failed to init response config";
        return -1;
    }

    return 0;
}

// Parse Redis reply recursively.
int parse_redis_reply(const brpc::RedisReply& redis_reply, rapidjson::Document& doc) {
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    if (redis_reply.is_nil()) {
        doc.SetNull();
    } else if (redis_reply.is_string()) {
        std::string str = redis_reply.data().as_string();
        doc.SetString(str.c_str(), str.length(), allocator);
    } else if (redis_reply.is_integer()) {
        doc.SetInt(redis_reply.integer());
    } else if (redis_reply.is_array()) {
        doc.SetArray();
        rapidjson::Document sub_doc(&allocator);
        for (size_t i = 0; i < redis_reply.size(); ++i) {
            if (parse_redis_reply(redis_reply[i], sub_doc) != 0) {
                return -1;
            }
            doc.PushBack(sub_doc, allocator);
        }
    } else if (redis_reply.is_error()) {
        std::string error_str = redis_reply.error_message();
        doc.SetString(error_str.c_str(), error_str.length(), allocator);
    } else {
        US_LOG(ERROR) << "Unknown redis reply type: [" << redis_reply.type() << "]";
        return -1;
    }

    return 0;
}

int RedisResponsePolicy::run(BackendController* cntl) const {
    RedisController* redis_cntl = static_cast<RedisController*>(cntl);
    BackendResponse& response = redis_cntl->response();
    brpc::RedisResponse& redis_response = redis_cntl->redis_response();
    expression::ExpressionContext response_context("redis response block", redis_cntl->context());
    rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

    response.SetArray();
    for (int i = 0; i < redis_response.reply_size(); ++i) {
        rapidjson::Value error(0);
        if (redis_response.reply(i).is_error()) {
            error.SetInt(1);
        }
        rapidjson::Document reply_value(&allocator);
        if (parse_redis_reply(redis_response.reply(i), reply_value) != 0) {
            US_LOG(ERROR) << "Failed to parse redis reply";
            return -1;
        }
        rapidjson::Value doc(rapidjson::kObjectType);
        doc.AddMember("error", error, allocator);
        doc.AddMember("data", reply_value, allocator);
        response.PushBack(doc, allocator);
    }

    US_DLOG(INFO) << "Response: " << json_encode(response);
    response_context.set_variable("response", response);

    if (_response_config.run(response_context) != 0) {
        US_LOG(ERROR) << "Failed to generate redis response config";
        return -1;
    }
    response_context.erase_variable("response");

    US_DLOG(INFO) << "Generated respone config: " << response_context.str();
    rapidjson::Value* output = response_context.get_variable("output");
    if (output == nullptr) {
        US_DLOG(ERROR) << "Not output found";
        return -1;
    }
    output->Swap(response);

    return 0;
}

} // namespace backend
} // namespace policy
} // namespace uskit
