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

#ifndef USKIT_POLICY_BACKEND_POLICY_H
#define USKIT_POLICY_BACKEND_POLICY_H

#include "config.pb.h"
#include "common.h"
#include "backend_controller.h"
#include "dynamic_config.h"
#include "backend.h"
#include "utils.h"

namespace uskit {
namespace policy {

// Base class of backend request policy.
class BackendRequestPolicy {
public:
    BackendRequestPolicy() {}
    virtual ~BackendRequestPolicy() {}

    virtual int init(const RequestConfig& config, const Backend* backend) {
        return 0;
    }
    virtual int run(BackendController* cntl) const = 0;
};

// Base class of backend response policy.
class BackendResponsePolicy {
public:
    BackendResponsePolicy() {}
    virtual ~BackendResponsePolicy() {}

    virtual int init(const ResponseConfig& config, const Backend* backend) {
        return 0;
    }
    virtual int run(BackendController* cntl) const = 0;
};

}  // namespace policy
}  // namespace uskit

#endif  // USKIT_POLICY_BACKEND_POLICY_H
