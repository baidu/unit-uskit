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

#ifndef USKIT_ERROR_H
#define USKIT_ERROR_H

#include <unordered_map>

namespace uskit {

// Error code definitions:
// 0 for OK.
// 400x for client-side errors.
// 500x for server-side errors.
enum ErrorCode {
    OK = 0,

    INVALID_JSON = 4001,
    MISSING_PARAM = 4002,
    USID_NOT_FOUND = 4003,

    INTERNAL_SERVER_ERROR = 5000,
};

// Error message for explaination.
const std::unordered_map<int, std::string> ErrorMessage {
    {OK, "OK"},
    {INVALID_JSON, "Invalid JSON"},
    {MISSING_PARAM, "Missing parameter"},
    {USID_NOT_FOUND, "usid not found"},
    {INTERNAL_SERVER_ERROR, "Internal server error"},
};

} // namespace uskit

#endif // USKIT_ERROR_H
