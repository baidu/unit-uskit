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

#include "flow_engine.h"
#include "policy/policy_manager.h"

namespace uskit {

FlowEngine::FlowEngine() : _intervene_config_json(rapidjson::kNullType) {}

FlowEngine::~FlowEngine() {}

int FlowEngine::member_init(const USConfig& config) {
    auto config_json = config.FindMember("backend");
    if (config_json == config.MemberEnd()) {
        US_LOG(ERROR) << "backend config not found in config: " << json_encode(config);
        return -1;
    } else if (!config_json->value.IsObject()) {
        US_LOG(ERROR) << "backend config required json object, [" << get_value_type(config_json->value)
                      << "] given";
        return -1;
    }
    std::string config_json_str = json_encode(config_json->value);
    BackendEngineConfig backend_config;
    if (!json2pb::JsonToProtoMessage(config_json_str, &backend_config)) {
        US_LOG(ERROR) << "backend config parsed from json to message error";
        return -1;
    }
    config_json = config.FindMember("rank");
    if (config_json == config.MemberEnd()) {
        US_LOG(ERROR) << "rank config not found in config: " << json_encode(config);
        return -1;
    } else if (!config_json->value.IsObject()) {
        US_LOG(ERROR) << "rank config required json object, [" << get_value_type(config_json->value)
                      << "] given";
        return -1;
    }
    config_json_str = json_encode(config_json->value);
    RankEngineConfig rank_config;
    if (!json2pb::JsonToProtoMessage(config_json_str, &rank_config)) {
        US_LOG(ERROR) << "rank config parsed from json to message error";
        return -1;
    }
    return member_init_by_message(backend_config, rank_config);
}

int FlowEngine::member_init(const std::string& root_dir, const std::string& usid) {
    // Assemble configuration paths.
    std::string backend_config_file(root_dir + "/" + usid + "/" + "backend.conf");
    std::string rank_config_file(root_dir + "/" + usid + "/" + "rank.conf");

    // Parse backend configuration.
    BackendEngineConfig backend_config;
    if (ReadProtoFromTextFile(backend_config_file, &backend_config) != 0) {
        LOG(ERROR) << "Failed to parse backend config, usid [" << usid << "]";
        return -1;
    }
    // Parse rank configuration.
    RankEngineConfig rank_config;
    if (ReadProtoFromTextFile(rank_config_file, &rank_config) != 0) {
        LOG(ERROR) << "Failed to parse rank config, usid [" << usid << "]";
        return -1;
    }

    return member_init_by_message(backend_config, rank_config);
}

int FlowEngine::member_init_by_message(
            const BackendEngineConfig& backend_config,
            const RankEngineConfig& rank_config) {
    _backend_engine = std::make_shared<BackendEngine>();
    // Initialize backend engine.
    LOG(INFO) << "Initializing backend engine";
    if (_backend_engine->init(backend_config) != 0) {
        LOG(ERROR) << "Failed to initialize backend engine";
        return -1;
    }
    _rank_engine = std::make_shared<RankEngine>();
    // Initialize rank engine.
    LOG(INFO) << "Initializing rank engine of usid";
    if (_rank_engine->init(rank_config) != 0) {
        LOG(ERROR) << "Failed to initialize rank engine, usid";
        return -1;
    }
    return 0;
}

int FlowEngine::init(const USConfig& config) {
    if (member_init(config) != 0) {
        return -1;
    }
    auto config_json = config.FindMember("flow");
    if (config_json == config.MemberEnd()) {
        US_LOG(ERROR) << "flow config not found in config: " << json_encode(config);
        return -1;
    } else if (!config_json->value.IsObject()) {
        US_LOG(ERROR) << "flow config required json object, [" << get_value_type(config_json->value)
                      << "] given";
        return -1;
    }
    std::string config_json_str = json_encode(config_json->value);
    FlowEngineConfig flow_config;
    if (!json2pb::JsonToProtoMessage(config_json_str, &flow_config)) {
        US_LOG(ERROR) << "flow config parsed from json to message error";
        return -1;
    }
    _curr_dir = "";
    auto iter = config.FindMember("intervene");
    if (iter == config.MemberEnd()) {
        _intervene_config_json.SetNull();
    } else {
        _intervene_config_json.CopyFrom(iter->value, _intervene_config_json.GetAllocator());
    }
    return init_by_message(flow_config);
}

int FlowEngine::init_by_message(const FlowEngineConfig& flow_config) {
    _flow_policy = std::unique_ptr<policy::FlowPolicy>(
            policy::PolicyManager::instance().get_flow_policy(flow_config.flow_policy()));
    if (!_flow_policy) {
        LOG(ERROR) << "Flow policy [" << flow_config.flow_policy() << "] not found";
        return -1;
    }

    _flow_policy->set_curr_dir(_curr_dir);

    if (!_intervene_config_json.IsNull()) {
        if (_flow_policy->init_intervene_by_json(_intervene_config_json) != 0) {
            LOG(ERROR) << "Failed to initialize intervene config in flow policy ["
                       << flow_config.flow_policy() << "]";
            return -1;
        }
    }

    if (_flow_policy->init(flow_config.flow()) != 0) {
        LOG(ERROR) << "Failed to initialize flow policy [" << flow_config.flow_policy() << "]";
        return -1;
    }

    _flow_policy->set_backend_engine(_backend_engine);
    _flow_policy->set_rank_engine(_rank_engine);
    return 0;
}

int FlowEngine::init(const std::string& root_dir, const std::string& usid) {
    // Initiate backend and rank engine
    if (member_init(root_dir, usid) != 0) {
        return -1;
    }

    std::string flow_config_file(root_dir + "/" + usid + "/" + "flow.conf");
    // Parse flow configuration.
    FlowEngineConfig flow_config;
    // JsonToProtoMessage, parse config from json
    if (ReadProtoFromTextFile(flow_config_file, &flow_config) != 0) {
        LOG(ERROR) << "Failed to parse flow config, usid [" << usid << "]";
        return -1;
    }
    _curr_dir = root_dir + "/" + usid + "/";
    return init_by_message(flow_config);
}

int FlowEngine::run(USRequest& request, USResponse& response) const {
    if (_flow_policy->run(request, response) != 0) {
        return -1;
    }
    return 0;
}

}  // namespace uskit
