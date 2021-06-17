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

#include "backend.h"
#include "butil.h"

namespace uskit {

Backend::Backend() : _channel(std::make_shared<BRPC_NAMESPACE::Channel>()) {}

Backend::~Backend() {}

int Backend::init(const BackendConfig& config) {
    // Setup backend channel options
    BRPC_NAMESPACE::ChannelOptions options;
    const std::string& protocol = config.protocol();
    options.protocol = protocol;
    if (config.has_connection_type()) {
        options.connection_type = config.connection_type();
    }
    if (config.has_connect_timeout_ms()) {
        options.connect_timeout_ms = config.connect_timeout_ms();
    }
    if (config.has_timeout_ms()) {
        options.timeout_ms = config.timeout_ms();
    }
    if (config.has_max_retry()) {
        options.max_retry = config.max_retry();
    }
    std::string load_balancer;
    if (config.has_load_balancer()) {
        load_balancer = config.load_balancer();
    }
    _is_dynamic = config.is_dynamic();

    // Initialize backend channel
    if (_channel->Init(config.server().c_str(), load_balancer.c_str(), &options) != 0) {
        LOG(ERROR) << "Failed to initialize channel of backend [" << config.name() << "]";
        return -1;
    }

    // Initialize request config template
    for (int i = 0; i < config.request_template_size(); ++i) {
        const RequestConfig& request_template = config.request_template(i);
        std::unique_ptr<BackendRequestConfig> request_config;
        if (protocol == "http") {
            if (!_is_dynamic) {
                request_config.reset(new HttpRequestConfig);
            } else {
                request_config.reset(new DynamicHttpRequestConfig);
            }
        } else if (protocol == "redis") {
            request_config.reset(new RedisRequestConfig);
        } else {
            LOG(ERROR) << "Unknown protocol [" << protocol << "]";
            return -1;
        }
        if (request_config->init(request_template) != 0) {
            LOG(ERROR) << "Failed to parse request config template [" << request_template.name()
                       << "] of backend [" << config.name() << "]";
            return -1;
        }
        _request_config_template_map.emplace(request_template.name(), std::move(request_config));
    }

    // Initialize response config template
    for (int j = 0; j < config.response_template_size(); ++j) {
        const ResponseConfig& response_template = config.response_template(j);
        BackendResponseConfig response_config;
        if (response_config.init(response_template) != 0) {
            LOG(ERROR) << "Failed to parse response config template [" << response_template.name()
                       << "] of backend [" << config.name() << "]";
            return -1;
        }
        _response_config_template_map.emplace(response_template.name(), std::move(response_config));
    }

    // Collect service names
    for (int i = 0; i < config.service_size(); ++i) {
        _services.emplace_back(config.service(i).name());
    }

    return 0;
}

std::shared_ptr<BRPC_NAMESPACE::Channel> Backend::channel() const {
    return _channel;
}

bool Backend::is_dynamic() const {
    return _is_dynamic;
}

const BRPC_NAMESPACE::AdaptiveProtocolType Backend::protocol() const {
    const BRPC_NAMESPACE::AdaptiveProtocolType protocol = _channel->options().protocol;
    return protocol;
}

const BackendRequestConfig* Backend::request_config(const std::string& name) const {
    if (_request_config_template_map.find(name) != _request_config_template_map.end()) {
        return _request_config_template_map.at(name).get();
    } else {
        return nullptr;
    }
}

const BackendResponseConfig* Backend::response_config(const std::string& name) const {
    if (_response_config_template_map.find(name) != _response_config_template_map.end()) {
        return &_response_config_template_map.at(name);
    } else {
        return nullptr;
    }
}

const std::vector<std::string>& Backend::services() const {
    return _services;
}

}  // namespace uskit
