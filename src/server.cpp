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

#include <gflags/gflags.h>
#include "brpc.h"
#include "us.pb.h"
#include "config.pb.h"
#include "thread_data.h"
#include "unified_scheduler_manager.h"
#include "utils.h"
#include "common.h"

DEFINE_int32(port, 8888, "TCP port of unified scheduler server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_string(us_conf, "./conf/us.conf", "Path of unified scheduler configuration file");
DEFINE_string(url_path, "/us", "URL path of unified scheduler service");
DEFINE_bool(log_to_file, false, "Log to file");

namespace uskit
{

class UnifiedSchedulerServiceImpl : public UnifiedSchedulerService {
public:
    virtual ~UnifiedSchedulerServiceImpl() {}
    virtual void run(google::protobuf::RpcController* cntl_base,
                     const HttpRequest*,
                     HttpResponse*,
                     google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        BRPC_NAMESPACE::ClosureGuard done_guard(done);

        BRPC_NAMESPACE::Controller* cntl =
            static_cast<BRPC_NAMESPACE::Controller*>(cntl_base);

        Timer total_tm("total_t_ms");
        total_tm.start();

        if (_us_manager.run(cntl) != 0) {
            US_LOG(ERROR) << "Failed to process request";
        }

        total_tm.stop();

        UnifiedSchedulerThreadData* td = static_cast<UnifiedSchedulerThreadData*>(BRPC_NAMESPACE::thread_local_data());
        US_LOG(NOTICE) << td->get_log();
        td->reset();
    }

    int init(const UnifiedSchedulerConfig& config) {
        if (_us_manager.init(config) != 0) {
            return -1;
        }

        return 0;
    }
    
private:
    UnifiedSchedulerManager _us_manager;
};

} // namespace uskit

int main(int argc, char *argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    uskit::UnifiedSchedulerConfig config;
    if (uskit::ReadProtoFromTextFile(FLAGS_us_conf, &config) != 0) {
        LOG(ERROR) << "Failed to parse unified scheduler config";
        return -1;
    }
    // Instance of your service.
    uskit::UnifiedSchedulerServiceImpl us_service;

    if (us_service.init(config) != 0) {
        LOG(ERROR) << "Failed to init unified scheduler from config";
        return -1;
    }

    // Generally you only need one Server.
    BRPC_NAMESPACE::Server server;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use BRPC_NAMESPACE::SERVER_OWNS_SERVICE.
    std::string url_path = FLAGS_url_path + " => run";
    if (server.AddService(&us_service, 
                          BRPC_NAMESPACE::SERVER_DOESNT_OWN_SERVICE,
                          url_path) != 0) {
        LOG(ERROR) << "Failed to add unified scheduler service";
        return -1;
    }

    // The factory to create UnifiedSchedulerThreadLocalData. Must be valid when server is running.
    uskit::UnifiedSchedulerThreadDataFactory thread_data_factory;

    // Start the server.
    BRPC_NAMESPACE::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.thread_local_data_factory = &thread_data_factory;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Failed to start server";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();

    return 0;
}
