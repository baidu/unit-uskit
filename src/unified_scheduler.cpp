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

#include "unified_scheduler.h"
#include "utils.h"

namespace uskit {

UnifiedScheduler::UnifiedScheduler() {
}

UnifiedScheduler::~UnifiedScheduler() {}

int UnifiedScheduler::init(const std::string& root_dir, const std::string& usid) {

    if (_flow_engine.init(root_dir, usid) != 0) {
        return -1;
    }
    return 0;
}

int UnifiedScheduler::init(const USConfig& config) {
    if (_flow_engine.init(config) != 0) {
        return -1;
    }
    return 0;
}

int UnifiedScheduler::run(USRequest& request, USResponse& response) const {
    // Run the chat flow.
    if (_flow_engine.run(request, response) != 0) {
        US_LOG(ERROR) << "Failed to run flow engine";
        return -1;
    }
    return 0;
}

} // namespace uskit
