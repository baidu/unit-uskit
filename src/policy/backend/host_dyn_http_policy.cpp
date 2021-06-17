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

#include "policy/backend/host_dyn_http_policy.h"
#include "utils.h"

namespace uskit {
namespace policy {
namespace backend {

int HostDynHttpRequestPolicy::call_method(
        BRPC_NAMESPACE::Controller& brpc_cntl,
        BackendController* cntl,
        expression::ExpressionContext& request_context) const {
    rapidjson::Value* host_ip_port = request_context.get_variable("host_ip_port");
    if (host_ip_port == nullptr) {
        US_LOG(ERROR) << "Required Host IP:Port";
        return -1;
    } else if (!host_ip_port->IsString()) {
        US_LOG(ERROR) << "Host IP:Port supposed to be string, [" << get_value_type(*host_ip_port)
                      << "] is given";
        return -1;
    }
    std::string server_address = host_ip_port->GetString();
    if (server_address.find(":") == server_address.npos) {
        US_LOG(ERROR) << "Host [" << server_address << "] is not in IP:PORT format";
        return -1;
    }

    _channel->Init(host_ip_port->GetString(), &(_backend->channel()->options()));
    _channel->CallMethod(nullptr, &brpc_cntl, nullptr, nullptr, cntl->_done.get());
    return 0;
}

}  // namespace backend
}  // namespace policy
}  // namespace uskit
