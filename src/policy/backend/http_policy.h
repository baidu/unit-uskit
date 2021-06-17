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

#ifndef USKIT_POLICY_BACKEND_HTTP_POLICY_H
#define USKIT_POLICY_BACKEND_HTTP_POLICY_H

#include "policy/backend_policy.h"
#include "dynamic_config.h"

namespace uskit {
namespace policy {
namespace backend {

// Default HTTP request policy.
class HttpRequestPolicy : public BackendRequestPolicy {
public:
    HttpRequestPolicy() : BackendRequestPolicy(), _backend(nullptr) {}
    int init(const RequestConfig& config, const Backend* backend);
    int run(BackendController* cntl) const;
    virtual int call_method(
            BRPC_NAMESPACE::Controller& brpc_cntl,
            BackendController* cntl,
            expression::ExpressionContext& request_context) const;

protected:
    HttpRequestConfig _request_config;
    const Backend* _backend;

private:
    std::shared_ptr<BRPC_NAMESPACE::Channel> _channel;
};

// Default HTTP response policy.
class HttpResponsePolicy : public BackendResponsePolicy {
public:
    int init(const ResponseConfig& config, const Backend* backend);
    int run(BackendController* cntl) const;

protected:
    BackendResponseConfig _response_config;
};

}  // namespace backend
}  // namespace policy
}  // namespace uskit

#endif  // USKIT_POLICY_BACKEND_HTTP_POLICY_H
