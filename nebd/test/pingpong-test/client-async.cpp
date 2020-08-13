#include <brpc/channel.h>
#include <brpc/controller.h>
#include <bthread/execution_queue.h>
#include <butil/time.h>
#include <bvar/bvar.h>
#include <gflags/gflags.h>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "nebd/proto/client.pb.h"
#include "src/common/concurrent/count_down_event.h"
#include "src/common/timeutility.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

brpc::Channel channel;
std::atomic<uint32_t> finishCount(1);
curve::common::ExpiredTime t;

bvar::LatencyRecorder rpcLatency("rpc_latency");

constexpr uint32_t BufSize = 4 * 1024;
char buffer[BufSize];

void TrivialDeleter(void* ptr) {}

const char* UnixSock = "/tmp/unix.pingpong.sock";
const char* TcpAddress = "localhost:12345";

DEFINE_int32(times, 100 * 100 * 100, "test times");
DEFINE_bool(attachment, true, "rpc with 4K attachment");
DEFINE_bool(tcp, true, "use tcp or unix");
DEFINE_string(tcpAddress, TcpAddress, "tcp address");

std::vector<double> lats;
bthread::Mutex mtx;

struct TestRequestClosure : public google::protobuf::Closure {
    void Run() override {
        std::unique_ptr<TestRequestClosure> selfGuard(this);

        if (cntl.Failed()) {
            std::cerr << "rpc failed, reason: " << cntl.ErrorText()
                      << std::endl;
            _exit(1);
            return;
        }

        auto cnt = finishCount.fetch_add(1, std::memory_order_relaxed);
        if (cnt % (FLAGS_times / 10) == 0) {
            auto costUs = t.ExpiredUs();
            std::cout << "count: " << cnt << ", cost " << (costUs / 1000.)
                      << " ms, " << (cnt * 1. / (costUs / 1000.)) * 1000
                      << " qps" << std::endl;
        }

        {
            std::lock_guard<bthread::Mutex> lk(mtx);
            lats.push_back(cntl.latency_us());
        }

        if (unlikely(cnt == FLAGS_times)) {
            double totalUs = std::accumulate(lats.begin(), lats.end(), 0.0);
            std::sort(lats.begin(), lats.end());
            std::cout << "avg latency us: " << (totalUs * 1.0) / lats.size()
                      << ", min: " << lats.front() << ", max: " << lats.back()
                      << "\n";
            _exit(0);
        }
    }

    brpc::Controller cntl;
    nebd::client::WriteRequest request;
    nebd::client::WriteResponse response;
};

void SendAsyncWriteRequest() {
    TestRequestClosure* done = new TestRequestClosure();
    done->request.set_fd(INT_MAX);
    done->request.set_offset(ULLONG_MAX);
    done->request.set_size(ULLONG_MAX);

    nebd::client::NebdFileService_Stub stub(&channel);
    done->cntl.set_timeout_ms(-1);

    // append 4K data
    if (FLAGS_attachment) {
        done->cntl.request_attachment().append_user_data(
            buffer, BufSize, TrivialDeleter);
    }

    stub.Write(&done->cntl, &done->request, &done->response, done);
}

void StartTest() {
    lats.reserve(FLAGS_times);

    for (int i = 0; i < FLAGS_times; ++i) {
        SendAsyncWriteRequest();
    };
}

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    ::memset(buffer, 1, sizeof(buffer));

    int ret = 0;
    if (FLAGS_tcp) {
        ret = channel.Init(FLAGS_tcpAddress.c_str(), nullptr);
    } else {
        ret = channel.InitWithSockFile(UnixSock, nullptr);
    }

    if (ret != 0) {
        std::cout << "channel Init failed";
        return -1;
    }

    std::cout << "times: " << FLAGS_times
              << ", with attachment: " << FLAGS_attachment
              << ", tcp: " << FLAGS_tcp << std::endl;

    StartTest();

    while (true) {
        sleep(1);
    }

    return 0;
}