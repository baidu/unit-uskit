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

#ifndef USKIT_CONTROLLER_CLOSURE_H
#define USKIT_CONTROLLER_CLOSURE_H
#include <string>
#include "common.h"
#include "expression/expression.h"
#include "utils.h"

namespace uskit {

// Forward declaration
class BackendController;
namespace policy {
class FlowPolicy;
class FlowPolicyHelper;
}
class DoNothingClosrue : public google::protobuf::Closure {
    void Run() {}
};

class BaseRPCClosure : public google::protobuf::Closure {
public:
    explicit BaseRPCClosure(
            BackendController* cntl,
            const std::string& name = "Base",
            const policy::FlowPolicy* flow_policy = nullptr);
    void Run() override;
    virtual void cancel(CallIdPriorityPair rpc_id) const = 0;

protected:
    std::string _closure_name;
    std::vector<CallIdPriorityPair> _rpc_ids;
    BackendController* _cntl;
    int _self_priority;
    const policy::FlowPolicy* _flow_policy;
};

class AllCancelClosure : public BaseRPCClosure {
public:
    AllCancelClosure(BackendController* cntl, const std::string& name) :
            BaseRPCClosure(cntl, name) {}
    void cancel(CallIdPriorityPair rpc_id) const override;
};

class PriorityClosure : public BaseRPCClosure {
public:
    PriorityClosure(BackendController* cntl, const std::string& name) :
            BaseRPCClosure(cntl, name) {}
    void cancel(CallIdPriorityPair rpc_id) const override;
};

class HierarchyClosure : public BaseRPCClosure {
public:
    HierarchyClosure(BackendController* cntl, const std::string& name) :
            BaseRPCClosure(cntl, name) {}
    void cancel(CallIdPriorityPair rpc_id) const override;
};

class GlobalClosure : public BaseRPCClosure {
public:
    GlobalClosure(BackendController* cntl, const std::string& name, const policy::FlowPolicy* flow_policy) :
            BaseRPCClosure(cntl, name, flow_policy) {}
    int ResponseCheck();
    bool ServiceFlagCheck();
    int GlobalCancel();
    int HelperInit(std::shared_ptr<policy::FlowPolicyHelper> helper);
    void Run() override;
    void cancel(CallIdPriorityPair rpc_id) const override;
    void cancel();
};

// Controller closure factory
std::unique_ptr<google::protobuf::Closure> build_controller_closure(
        const std::string& cancel_order,
        BackendController* cntl,
        const policy::FlowPolicy* flow_policy = nullptr);

}  // namespace uskit

#endif