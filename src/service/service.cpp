#include "service/service.hpp"
#include "ipc/pipe_server.hpp"
#include "common/log.hpp"
#include <Windows.h>
#include <atomic>
#include <memory>
#include <string>

namespace {
  std::unique_ptr<ipc::PipeServer> g_pipe;
  std::atomic<bool> g_running{true};
}

static std::string handle_status(const std::string&) {
  return R"({"state":"running"})";
}
static std::string handle_stop(const std::string&) {
  g_running = false;
  return R"({"ok":true})";
}
// Stubs for now:
static std::string handle_connect(const std::string&)   { return R"({"ok":true})"; }
static std::string handle_disconnect(const std::string&){ return R"({"ok":true})"; }

bool svc::run() {
  // Initialize logging (console run or SCM run both hit here)
  logx::init(L"C:\\ProgramData\\VpnClient\\vpn-service.log");
  logx::info(L"VPN Service starting");

  g_pipe = std::make_unique<ipc::PipeServer>(L"vpnclient.pipe");
  g_pipe->on("Status",     handle_status);
  g_pipe->on("Stop",       handle_stop);
  g_pipe->on("Connect",    handle_connect);
  g_pipe->on("Disconnect", handle_disconnect);
  g_pipe->start();
  logx::info(L"IPC server started on \\\\.\\pipe\\vpnclient.pipe");

  // Console Ctrl+C handler for clean shutdown
  SetConsoleCtrlHandler([](DWORD sig)->BOOL {
    if (sig == CTRL_C_EVENT || sig == CTRL_BREAK_EVENT || sig == CTRL_CLOSE_EVENT) {
      g_running = false;
      return TRUE;
    }
    return FALSE;
  }, TRUE);

  // Main loop
  while (g_running) {
    Sleep(100);
  }

  // Teardown
  g_pipe->stop();
  logx::info(L"VPN Service stopped");
  return true;
}