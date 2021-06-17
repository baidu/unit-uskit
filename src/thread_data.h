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

#ifndef USKIT_THREAD_DATA_H
#define USKIT_THREAD_DATA_H

#include <string>
#include <vector>

#include "brpc.h"
#include "butil.h"

namespace uskit {

// Thread local data for log entries.
class UnifiedSchedulerThreadData {
public:
    void reset() {
        _log_entries.clear();
    }

    void set_logid(std::string& logid) {
        _logid = logid;
    }

    const std::string& logid() {
        return _logid;
    }

    void add_log_entry(const std::string& key, const std::string& value) {
        _log_entries.push_back(key + "=" + value);
    }

    void add_log_entry(const std::string& key, int value) {
        add_log_entry(key, std::to_string(value));
    }

    void add_log_entry(const std::string& key, std::vector<std::string>& value) {
        add_log_entry(key, JoinString(value, ','));
    }

    const std::string get_log() {
        return JoinString(_log_entries, ' ');
    }

private:
    std::string _logid;
    std::vector<std::string> _log_entries;
};

// Thread local data factory.
class UnifiedSchedulerThreadDataFactory : public BRPC_NAMESPACE::DataFactory {
public:
    void* CreateData() const {
        return new UnifiedSchedulerThreadData;
    }

    void DestroyData(void* d) const {
        delete static_cast<UnifiedSchedulerThreadData*>(d);
    }
};

}  // namespace uskit

#endif  // USKIT_THREAD_DATA_H
