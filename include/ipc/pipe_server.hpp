#pragma once
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>

namespace ipc {
    // Handler receives the raw JSON request and returns a JSON string reply.
    using Handler = std::function<std::string(const std::string& jsonReq)>;

    class PipeServer {
    public:
        explicit PipeServer(const std::wstring& name);
        ~PipeServer();

        // Start/stop the background accept loop
        void start();
        void stop();

        // Register a method handler, e.g. "Status", "Connect"
        void on(const std::string& method, Handler h);

    private:
        std::wstring pipeName_;                         // e.g. \\.\pipe\vpnclient.pipe
        std::unordered_map<std::string, Handler> handlers_;
        std::atomic<bool> running_{ false };
        std::thread th_;
    };
}