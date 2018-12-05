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

#ifndef USKIT_COMMON_H
#define USKIT_COMMON_H

#include <rapidjson/document.h>
#include <butil/logging.h>
#include "thread_data.h"

namespace uskit {

typedef rapidjson::Document USRequest;
typedef rapidjson::Document USResponse;

typedef rapidjson::Document BackendResponse;

typedef rapidjson::Value RankCandidate;
typedef rapidjson::Value RankResult;

} // namespace uskit

// Wrappter for logging with logid tracking
#define US_LOG(severity) \
    LOG(severity) << "logid=" << (brpc::thread_local_data() == nullptr ? "" : \
        (static_cast<uskit::UnifiedSchedulerThreadData*>(brpc::thread_local_data()))->logid()) \
        << " "

// Wrappter for debug logging with logid tracking
#define US_DLOG(severity) \
    DLOG(severity) << "logid=" << (brpc::thread_local_data() == nullptr ? "" : \
        (static_cast<uskit::UnifiedSchedulerThreadData*>(brpc::thread_local_data()))->logid()) \
        << " "

#endif // USKIT_COMMON_H
