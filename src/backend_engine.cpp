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

#include <butil/logging.h>
#include <butil/strings/string_util.h>
#include "backend_engine.h"
#include "backend_controller.h"
#include "utils.h"
#include "thread_data.h"

namespace uskit
{

BackendEngine::BackendEngine() {
}

BackendEngine::~BackendEngine() {
}

int BackendEngine::init(const BackendEngineConfig &config) {
    std::vector<std::string> backends;
    // Initialize backends
    for (int i = 0; i < config.backend_size(); ++i) {
        const BackendConfig &backend_config = config.backend(i);
        Backend backend;
        if (backend.init(backend_config) != 0) {
            LOG(ERROR) << "Failed to init backend [" << backend_config.name() << "]";
            return -1;
        }
        _backend_map.emplace(backend_config.name(), std::move(backend));
        // Initialize backend services
        for (int j = 0; j < backend_config.service_size(); ++j) {
            const ServiceConfig &service_config = backend_config.service(j);
            BackendService service;
            if (service.init(service_config, &_backend_map[backend_config.name()]) != 0) {
                LOG(ERROR) << "Failed to initialize service ["
                           << service_config.name() << "]";
                return -1;
            }
            _service_map.emplace(service_config.name(), std::move(service));
        }
        backends.emplace_back(backend_config.name());
    }

    LOG(INFO) << "Initialized backends: " << JoinString(backends, ',');

    return 0;
}

int BackendEngine::run(const std::vector<std::string> &recall_services,
                       expression::ExpressionContext& context) const {
    std::string recall_services_str = JoinString(recall_services, ',');
    UnifiedSchedulerThreadData* td = static_cast<UnifiedSchedulerThreadData*>(brpc::thread_local_data());

    std::vector<std::unique_ptr<BackendController>> cntls;

    Timer recall_tm("recall_total_t_ms(" + recall_services_str + ")");
    recall_tm.start();
    Timer build_request_tm("build_request_total_t_ms(" + recall_services_str + ")");
    build_request_tm.start();
    std::vector<std::string> build_request_result;
    for (std::vector<std::string>::const_iterator iter = recall_services.begin();
            iter != recall_services.end(); ++iter) {
        Timer tm("build_request_t_ms(" + *iter + ")");
        tm.start();
        auto service_iter = _service_map.find(*iter);
        if (service_iter == _service_map.end()) {
            US_LOG(WARNING) << "Unknown service [" << *iter << "], skipping";
        } else {
            // Build backend controller
            std::unique_ptr<BackendController> cntl(build_backend_controller(&service_iter->second, context));
            if (cntl->build_request() == 0) {
                build_request_result.push_back(cntl->service_name());
                cntls.emplace_back(std::move(cntl));
            }
        }
        tm.stop();
    }
    td->add_log_entry("build_request_result(" + recall_services_str + ")", build_request_result);
    build_request_tm.stop();

    std::vector<std::string> recall_result;
    // Wait util all service calls finish
    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        brpc::Controller& brpc_cntl = (*iter)->brpc_controller();
        brpc::Join(brpc_cntl.call_id());
    }

    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        brpc::Controller& brpc_cntl = cntl.brpc_controller();
        td->add_log_entry("recall_t_ms(" + cntl.service_name() + ")", brpc_cntl.latency_us() / 1000);
        if (!brpc_cntl.Failed()) {
            recall_result.push_back(cntl.service_name());
        }
    }
    td->add_log_entry("recall_result(" + recall_services_str + ")", recall_result);
    recall_tm.stop();

    Timer parse_response_tm("parse_response_total_t_ms(" + recall_services_str + ")");
    parse_response_tm.start();
    std::vector<std::string> parse_response_result;
    // Collect service responses
    rapidjson::Value success_recall_services(rapidjson::kArrayType);
    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        brpc::Controller& brpc_cntl = cntl.brpc_controller();
        Timer tm("parse_response_t_ms(" + cntl.service_name() + ")");
        tm.start();

        if (brpc_cntl.Failed()) {
            US_LOG(WARNING) << "Failed to receive response from service ["
                << cntl.service_name() << "]"
                << " remote_server=" << brpc_cntl.remote_side()
                << " latency=" << brpc_cntl.latency_us() << "us"
                << " error_msg=" << brpc_cntl.ErrorText();
            // Skip parsing
            continue;
        }
        US_LOG(INFO) << "Received response from service ["
            << cntl.service_name() << "]"
            << " remote_server=" << brpc_cntl.remote_side()
            << " latency=" << brpc_cntl.latency_us() << "us";
        // Parse response
        if (cntl.parse_response() == 0) {
            rapidjson::Value* backend_result = context.get_variable("backend");
            rapidjson::Value service_name;
            service_name.SetString(cntl.service_name().c_str(), cntl.service_name().length(), context.allocator());
            backend_result->AddMember(service_name, cntl.response(), context.allocator());
            service_name.SetString(cntl.service_name().c_str(), cntl.service_name().length(), context.allocator());
            success_recall_services.PushBack(service_name, context.allocator());
            parse_response_result.push_back(cntl.service_name());
        }
        tm.stop();
    }
    // Setup variable `recall'
    context.set_variable("recall", success_recall_services);
    td->add_log_entry("parse_response_result(" + recall_services_str + ")", parse_response_result);

    parse_response_tm.stop();

    return 0;
}

} // namespace uskit
