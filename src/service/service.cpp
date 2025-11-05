// src/service/service.cpp
#include "service/service.hpp"
#include "ipc/pipe_server.hpp"
#include "common/log.hpp"

#include <windows.h>
#include <atomic>
#include <memory>
#include <string>

using ipc::PipeServer;

namespace {
  HANDLE g_stopEvent = nullptr;                 // manual-reset event
  std::unique_ptr<PipeServer> g_server;
}

// --- handlers ---
static std::string h_status(const std::string&) {
  return R"({"status":"running"})";
}

static std::string h_disconnect(const std::string&) {
  // TODO: tear down WG/OpenVPN, revert DNS/routes, disable killswitch (if needed)
  logx::info(L"disconnect requested");
  return R"({"ok":true})";
}

static std::string h_connect(const std::string& req) {
  // Minimal demo: in the future parse JSON for host/pub/priv/dns.
  // For now just acknowledge.
  logx::info(L"connect requested");
  return R"({"ok":true})";
}

static std::string h_stop(const std::string&) {
  logx::info(L"stop requested");
  if (g_stopEvent) SetEvent(g_stopEvent);
  return R"({"ok":true})";
}

bool svc::run() {
  if (!g_stopEvent) {
    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent) {
      logx::error(L"CreateEventW failed");
      return false;
    }
  }

  g_server = std::make_unique<PipeServer>(L"vpnsvc");
  g_server->on("status",     h_status);
  g_server->on("connect",    h_connect);
  g_server->on("disconnect", h_disconnect);
  g_server->on("stop",       h_stop);
  g_server->start();

  logx::info(L"service running; waiting for stop");
  WaitForSingleObject(g_stopEvent, INFINITE);

  if (g_server) { g_server->stop(); g_server.reset(); }
  return true;
}

void svc::stop() {
  if (g_stopEvent) SetEvent(g_stopEvent);
}
