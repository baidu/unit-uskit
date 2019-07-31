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

#include "butil.h"
#include "backend_engine.h"
#include "backend_controller.h"
#include "utils.h"
#include "thread_data.h"

namespace uskit {

BackendEngine::BackendEngine() {}

BackendEngine::~BackendEngine() {}

int BackendEngine::init(const BackendEngineConfig& config) {
    std::vector<std::string> backends;
    // Initialize backends
    size_t service_beg = 0;
    for (int i = 0; i < config.backend_size(); ++i) {
        const BackendConfig& backend_config = config.backend(i);
        Backend backend;
        if (backend.init(backend_config) != 0) {
            LOG(ERROR) << "Failed to init backend [" << backend_config.name() << "]";
            return -1;
        }
        _backend_map.emplace(backend_config.name(), std::move(backend));
        // Initialize backend services
        for (int j = 0; j < backend_config.service_size(); ++j) {
            const ServiceConfig& service_config = backend_config.service(j);
            BackendService service;
            if (service.init(service_config, &_backend_map[backend_config.name()]) != 0) {
                LOG(ERROR) << "Failed to initialize service [" << service_config.name() << "]";
                return -1;
            }
            _service_map.emplace(service_config.name(), std::move(service));
            _service_context_index.emplace(service_config.name(), service_beg++);
        }
        backends.emplace_back(backend_config.name());
    }

    LOG(INFO) << "Initialized backends: " << JoinString(backends, ',');

    return 0;
}

int BackendEngine::run(
        const std::vector<std::string>& recall_services,
        expression::ExpressionContext& context) const {
    std::string recall_services_str = JoinString(recall_services, ',');
    UnifiedSchedulerThreadData* td =
            static_cast<UnifiedSchedulerThreadData*>(BRPC_NAMESPACE::thread_local_data());

    std::vector<std::unique_ptr<BackendController>> cntls;

    Timer recall_tm("recall_total_t_ms(" + recall_services_str + ")");
    Timer build_request_tm("build_request_total_t_ms(" + recall_services_str + ")");
    if (recall_services.size() != 0) {
        recall_tm.start();
        build_request_tm.start();
    }
    std::vector<std::string> build_request_result;
    for (std::vector<std::string>::const_iterator iter = recall_services.begin();
         iter != recall_services.end();
         ++iter) {
        Timer tm("build_request_t_ms(" + *iter + ")");
        tm.start();
        auto service_iter = _service_map.find(*iter);
        if (service_iter == _service_map.end()) {
            US_LOG(WARNING) << "Unknown service [" << *iter << "], skipping";
        } else {
            // Build backend controller
            std::unique_ptr<BackendController> cntl(
                    build_backend_controller(&service_iter->second, context));
            if (cntl->build_request() == 0) {
                build_request_result.push_back(cntl->service_name());
                cntls.emplace_back(std::move(cntl));
            }
        }
        tm.stop();
    }
    if (recall_services.size() != 0) {
        td->add_log_entry("build_request_result(" + recall_services_str + ")", build_request_result);
        build_request_tm.stop();
    }

    std::vector<std::string> recall_result;
    // Wait util all service calls finish
    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        if (DynamicHTTPController* dhc = dynamic_cast<DynamicHTTPController*>(iter->get())) {
            for (auto brpc_iter = dhc->brpc_controller_list().begin();
                 brpc_iter != dhc->brpc_controller_list().end();
                 ++brpc_iter) {
                BRPC_NAMESPACE::Join(brpc_iter->get()->call_id());
            }
        } else {
            BRPC_NAMESPACE::Controller& brpc_cntl = (*iter)->brpc_controller();
            BRPC_NAMESPACE::Join(brpc_cntl.call_id());
        }
    }

    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        if (DynamicHTTPController* dhc = dynamic_cast<DynamicHTTPController*>(iter->get())) {
            int64_t latency_time = 0;
            bool failed = false;
            for (auto brpc_iter = dhc->brpc_controller_list().begin();
                 brpc_iter != dhc->brpc_controller_list().end();
                 ++brpc_iter) {
                latency_time += brpc_iter->get()->latency_us() / 1000;
                if (!brpc_iter->get()->Failed()) {
                    continue;
                }
                failed = true;
            }
            td->add_log_entry("recall_t_ms(" + cntl.service_name() + ")", latency_time);
            if (!failed) {
                recall_result.push_back(cntl.service_name());
            }
        } else {
            BRPC_NAMESPACE::Controller& brpc_cntl = cntl.brpc_controller();
            td->add_log_entry(
                    "recall_t_ms(" + cntl.service_name() + ")", brpc_cntl.latency_us() / 1000);
            if (!brpc_cntl.Failed()) {
                recall_result.push_back(cntl.service_name());
            }
        }
    }

    Timer parse_response_tm("parse_response_total_t_ms(" + recall_services_str + ")");
    if (recall_services.size() != 0) {
        td->add_log_entry("recall_result(" + recall_services_str + ")", recall_result);
        recall_tm.stop();
        parse_response_tm.start();
    }
    std::vector<std::string> parse_response_result;
    // Collect service responses
    rapidjson::Value success_recall_services(rapidjson::kArrayType);
    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        BRPC_NAMESPACE::Controller& brpc_cntl = cntl.brpc_controller();
        Timer tm("parse_response_t_ms(" + cntl.service_name() + ")");
        tm.start();

        if (brpc_cntl.Failed()) {
            US_LOG(WARNING) << "Failed to receive response from service [" << cntl.service_name()
                            << "]"
                            << " remote_server=" << brpc_cntl.remote_side()
                            << " latency=" << brpc_cntl.latency_us() << "us"
                            << " error_msg=" << brpc_cntl.ErrorText();
            // Skip parsing
            continue;
        }
        US_LOG(INFO) << "Received response from service [" << cntl.service_name() << "]"
                     << " remote_server=" << brpc_cntl.remote_side()
                     << " latency=" << brpc_cntl.latency_us() << "us";
        // Parse response
        if (cntl.parse_response() == 0) {
            rapidjson::Value* backend_result = context.get_variable("backend");
            rapidjson::Value service_name;
            service_name.SetString(
                    cntl.service_name().c_str(), cntl.service_name().length(), context.allocator());
            backend_result->AddMember(service_name, cntl.response(), context.allocator());
            service_name.SetString(
                    cntl.service_name().c_str(), cntl.service_name().length(), context.allocator());
            success_recall_services.PushBack(service_name, context.allocator());
            parse_response_result.push_back(cntl.service_name());
        }
        tm.stop();
    }
    // Setup variable `recall'
    context.set_variable("recall", success_recall_services);
    if (recall_services.size() != 0) {
        td->add_log_entry("parse_response_result(" + recall_services_str + ")", parse_response_result);
        parse_response_tm.stop();
    }

    return 0;
}

int BackendEngine::run(
        const std::vector<std::pair<std::string, int>>& recall_services,
        expression::ExpressionContext& context,
        const std::string cancel_order) const {
    std::vector<std::string> recall_services_strs;
    for (auto& rec : recall_services) {
        recall_services_strs.push_back(rec.first);
    }
    std::string recall_services_str = JoinString(recall_services_strs, ',');
    UnifiedSchedulerThreadData* td =
            static_cast<UnifiedSchedulerThreadData*>(BRPC_NAMESPACE::thread_local_data());

    std::vector<std::unique_ptr<BackendController>> cntls;

    Timer recall_tm("recall_total_t_ms(" + recall_services_str + ")");
    recall_tm.start();
    Timer build_request_tm("build_request_total_t_ms(" + recall_services_str + ")");
    build_request_tm.start();
    std::vector<std::string> build_request_result;
    std::vector<CallIdPriorityPair> cntls_call_ids;
    for (std::vector<std::pair<std::string, int>>::const_iterator iter = recall_services.begin();
         iter != recall_services.end();
         ++iter) {
        Timer tm("build_request_t_ms(" + iter->first + ")");
        tm.start();
        auto service_iter = _service_map.find(iter->first);
        if (service_iter == _service_map.end()) {
            US_LOG(WARNING) << "Unknown service [" << iter->first << "], skipping";
        } else {
            // Build backend controller
            std::unique_ptr<BackendController> cntl(
                    build_backend_controller(&service_iter->second, context));
            cntls_call_ids.push_back(
                    CallIdPriorityPair(cntl->brpc_controller().call_id(), iter->second));
            cntl->set_cancel_order(cancel_order);
            cntl->set_priority(iter->second);
            cntls.emplace_back(std::move(cntl));
        }
        tm.stop();
    }
    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        cntl.set_call_ids(cntls_call_ids);
        US_DLOG(INFO) << "start build cntl: " << cntl.brpc_controller().call_id();
        if (cntl.build_request() == 0) {
            US_DLOG(INFO) << "finish cntl: " << cntl.brpc_controller().call_id();
            build_request_result.push_back(cntl.service_name());
        }
    }

    td->add_log_entry("build_request_result(" + recall_services_str + ")", build_request_result);
    build_request_tm.stop();

    std::vector<std::string> recall_result;
    // Wait util all service calls finish
    for (auto iter = cntls_call_ids.begin(); iter != cntls_call_ids.end(); ++iter) {
        BRPC_NAMESPACE::Join(iter->first);
    }

    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        BRPC_NAMESPACE::Controller& brpc_cntl = cntl.brpc_controller();
        td->add_log_entry(
                "recall_t_ms(" + cntl.service_name() + ")", brpc_cntl.latency_us() / 1000);
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
        BRPC_NAMESPACE::Controller& brpc_cntl = cntl.brpc_controller();
        Timer tm("parse_response_t_ms(" + cntl.service_name() + ")");
        tm.start();

        if (brpc_cntl.Failed()) {
            US_LOG(WARNING) << "Failed to receive response from service [" << cntl.service_name()
                            << "]"
                            << " remote_server=" << brpc_cntl.remote_side()
                            << " latency=" << brpc_cntl.latency_us() << "us"
                            << " error_msg=" << brpc_cntl.ErrorText();
            // Skip parsing
            continue;
        }
        US_LOG(INFO) << "Received response from service [" << cntl.service_name() << "]"
                     << " remote_server=" << brpc_cntl.remote_side()
                     << " latency=" << brpc_cntl.latency_us() << "us";
        // Parse response
        if (cntl.parse_response() == 0) {
            rapidjson::Value* backend_result = context.get_variable("backend");
            rapidjson::Value service_name;
            service_name.SetString(
                    cntl.service_name().c_str(), cntl.service_name().length(), context.allocator());
            backend_result->AddMember(service_name, cntl.response(), context.allocator());
            service_name.SetString(
                    cntl.service_name().c_str(), cntl.service_name().length(), context.allocator());
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

int BackendEngine::run(
        const FlowRecallConfig& recall_config,
        std::vector<expression::ExpressionContext*>& context,
        const std::unordered_map<std::string, FlowConfig>* flow_map,
        const RankEngine* rank_engine,
        std::shared_ptr<CallIdsVecThreadSafe> ids_ptr) const {
    std::vector<std::string> recall_services_strs;
    std::vector<std::pair<std::string, int>> recall_services = recall_config.get_recall_services();
    for (auto& rec : recall_services) {
        recall_services_strs.push_back(rec.first);
    }
    std::string recall_services_str = JoinString(recall_services_strs, ',');

    std::vector<std::unique_ptr<BackendController>> cntls;

    Timer recall_tm("recall_total_t_ms(" + recall_services_str + ")");
    recall_tm.start();
    Timer build_request_tm("build_request_total_t_ms(" + recall_services_str + ")");
    build_request_tm.start();
    std::unordered_set<std::string> build_request_result_set;
    std::vector<std::string> build_request_result;
    std::vector<CallIdPriorityPair> cntls_call_ids;
    std::unordered_map<size_t, const std::string> callid_serv_map;
    for (std::vector<std::pair<std::string, int>>::const_iterator iter = recall_services.begin();
         iter != recall_services.end();
         ++iter) {
        Timer tm("build_request_t_ms(" + iter->first + ")");
        tm.start();
        auto service_iter = _service_map.find(iter->first);
        if (service_iter == _service_map.end()) {
            US_LOG(WARNING) << "Unknown service [" << iter->first << "], skipping";
        } else {
            // Build backend controller
            size_t service_index = _service_context_index.find(iter->first)->second;
            std::unique_ptr<BackendController> cntl(
                    build_backend_controller(&service_iter->second, *(context[service_index])));
            std::vector<BRPC_NAMESPACE::CallId> dhc_call_ids;
            if (DynamicHTTPController* dhc = dynamic_cast<DynamicHTTPController*>(cntl.get())) {
                US_DLOG(INFO) << "dynamic http controller";
            } else {
                cntls_call_ids.push_back(
                        CallIdPriorityPair(cntl->brpc_controller().call_id(), iter->second));
                callid_serv_map.emplace(cntls_call_ids.size() - 1, iter->first);
            }
            cntl->set_cancel_order(recall_config.get_cancel_order());
            cntl->set_priority(iter->second);
            if (ids_ptr) {
                int ret = ids_ptr->set_call_id(service_index, cntl->brpc_controller().call_id());
                if (ret == -1) {
                    US_LOG(ERROR) << "Unable to set call id to call_ids_ptr";
                    return -1;
                } else if (ret == 1) {
                    US_LOG(WARNING) << "Flow is ready to quit";
                    return 0;
                }
            }
            cntl->_call_ids_ptr = ids_ptr;
            std::unordered_map<std::string, std::string> _recall_next =
                    recall_config.get_recall_next();
            cntl->_flow_context_array = &context;
            cntl->_flow_name = recall_config.get_flow_name();
            for (auto iter : _service_context_index) {
                cntl->_service_context_index.emplace(iter.first, iter.second);
            }

            auto _next_iter = _recall_next.find(iter->first);
            if (_next_iter != _recall_next.end()) {
                cntl->set_recall_next(_next_iter->second);
                US_DLOG(INFO) << "recall next map: " << _next_iter->first << " "
                              << _next_iter->second;
            }

            cntls.emplace_back(std::move(cntl));
        }
        tm.stop();
    }
    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        cntl.set_call_ids(cntls_call_ids);
        if (cntl.build_request(this, flow_map, rank_engine) == 0) {
            build_request_result.push_back(cntl.service_name());
            build_request_result_set.insert(cntl.service_name());
            std::vector<BRPC_NAMESPACE::CallId> dhc_call_ids;
            if (DynamicHTTPController* dhc = dynamic_cast<DynamicHTTPController*>(&cntl)) {
                for (auto brpc_iter = dhc->brpc_controller_list().begin();
                     brpc_iter != dhc->brpc_controller_list().end();
                     ++brpc_iter) {
                    dhc_call_ids.push_back(brpc_iter->get()->call_id());
                    cntls_call_ids.push_back(
                            CallIdPriorityPair(brpc_iter->get()->call_id(), cntl.get_priority()));
                    callid_serv_map.emplace(cntls_call_ids.size() - 1, cntl.service_name());
                }
                size_t service_index = _service_context_index.find(cntl.service_name())->second;
                int ret = ids_ptr->set_call_id(service_index, dhc_call_ids);
                if (ret == -1) {
                    US_LOG(ERROR) << "Unable to set call id to call_ids_ptr";
                    return -1;
                } else if (ret == 1) {
                    US_LOG(WARNING) << "Flow is ready to quit";
                    return 0;
                }
            }
        }
    }

    build_request_tm.stop();

    std::vector<std::string> recall_result;
    // Wait util all service calls finish
    for (size_t index = 0; index < cntls_call_ids.size(); ++index) {
        std::string service_name = callid_serv_map.find(index)->second;
        if (build_request_result_set.find(service_name) != build_request_result_set.end()) {
            BRPC_NAMESPACE::Join(cntls_call_ids[index].first);
        }
    }

    for (auto iter = cntls.begin(); iter != cntls.end(); ++iter) {
        BackendController& cntl = **iter;
        BRPC_NAMESPACE::Controller& brpc_cntl = cntl.brpc_controller();
        if (!brpc_cntl.Failed()) {
            recall_result.push_back(cntl.service_name());
        }
    }
    recall_tm.stop();
    return 0;
}

size_t BackendEngine::get_service_size() const {
    return _service_context_index.size();
}

size_t BackendEngine::get_service_index(std::string service_name) const {
    return _service_context_index.find(service_name)->second;
}

bool BackendEngine::hse_service(std::string service_name) const {
    return (_service_context_index.find(service_name) != _service_context_index.end());
}

}  // namespace uskit
