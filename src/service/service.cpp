#include "service/service.hpp"
#include "ipc/pipe_server.hpp"
#include "ipc/messages.hpp"
#include "vpn/wireguard_handler.hpp"
#include "net/route_dns.hpp"
#include "net/wfp_manager.hpp"
#include "common/log.hpp"

#include <Windows.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace {
  std::unique_ptr<ipc::PipeServer> g_pipe;
  std::atomic<bool> g_running{true};
  std::atomic<bool> g_connected{false};

  vpn::WireGuardHandler g_wg;
  net::WfpManager       g_wfp;
  std::wstring          g_ifName = L"vg0";
  std::vector<std::wstring> g_lastDns;

  // super-naive helpers just to pull a few fields from a JSON-ish string
  static std::string grab(const std::string& s, const std::string& key) {
    // finds "key":"value"
    auto p = s.find("\"" + key + "\"");
    if (p == std::string::npos) return {};
    p = s.find(':', p); if (p == std::string::npos) return {};
    p = s.find('"', p); if (p == std::string::npos) return {};
    auto q = s.find('"', p+1); if (q == std::string::npos) return {};
    return s.substr(p+1, q-(p+1));
  }
  static std::vector<std::wstring> grabArrayW(const std::string& s, const std::string& key) {
    // finds "key":["a","b"]
    std::vector<std::wstring> r;
    auto p = s.find("\"" + key + "\"");
    if (p == std::string::npos) return r;
    p = s.find('[', p); if (p == std::string::npos) return r;
    auto q = s.find(']', p); if (q == std::string::npos) return r;
    std::string body = s.substr(p+1, q-(p+1)); // between [ and ]

    size_t i=0;
    while (true) {
      auto a = body.find('"', i);
      if (a == std::string::npos) break;
      auto b = body.find('"', a+1);
      if (b == std::string::npos) break;
      std::string item = body.substr(a+1, b-(a+1));
      r.emplace_back(std::wstring(item.begin(), item.end()));
      i = b+1;
    }
    return r;
  }
}

// ---------------- Handlers ----------------

static std::string handle_status(const std::string&) {
  return g_connected ? msgs::status_connected() : msgs::status_idle();
}

static std::string handle_stop(const std::string&) {
  logx::info(L"Stop requested via IPC");
  g_running = false;
  return msgs::ok();
}

static std::string handle_disconnect(const std::string&) {
  logx::info(L"Disconnect requested");
  // kill switch off first so we don't trap ourselves
  g_wfp.disableKillSwitch();
  net::revertDns(g_ifName);
  net::revertRoutes();
  g_wg.down();
  g_connected = false;
  logx::info(L"Disconnected");
  return msgs::ok();
}

static std::string handle_connect(const std::string& req) {
  // Extract minimal fields from JSON-ish string
  const auto endpoint      = grab(req, "endpoint");
  const auto peerPublicKey = grab(req, "peerPublicKey");
  const auto privateKey    = grab(req, "privateKey");
  const auto allowedIps    = grab(req, "allowedIps");
  const auto dnsServers    = grabArrayW(req, "dns");

  // Log what we got (avoid logging private key)
  std::wstring wep(endpoint.begin(), endpoint.end());
  std::wstring wpeer(peerPublicKey.begin(), peerPublicKey.end());
  std::wstring wallowed(allowedIps.begin(), allowedIps.end());
  logx::info(L"Connect: endpoint=" + wep + L" peer=" + wpeer + L" allowed=" + wallowed);

  // WG: create/apply/up (stubbed inside handler for now)
  vpn::WgConfig cfg;
  cfg.privateKey     = privateKey;
  cfg.peerPublicKey  = peerPublicKey;
  cfg.endpoint       = endpoint;
  cfg.allowedIps     = allowedIps;
  g_wg.createInterface(g_ifName);
  g_wg.applyConfig(cfg);
  g_wg.up();

  // Set default route via WG + DNS
  net::setDefaultRouteToInterface(g_ifName);
  if (!dnsServers.empty()) {
    g_lastDns = dnsServers;
    net::setDnsServersForInterface(g_ifName, g_lastDns);
  }

  // Enable kill switch (allow control + DNS in future)
  g_wfp.enableKillSwitch(L"", {});

  g_connected = true;
  logx::info(L"Connect complete");
  return msgs::ok();
}

// --------------- Service run loop ---------------

bool svc::run() {
  logx::info(L"VPN Service starting (Day 3)");

  g_wfp.init();

  g_pipe = std::make_unique<ipc::PipeServer>(L"vpnclient.pipe");
  g_pipe->on("Status",     handle_status);
  g_pipe->on("Stop",       handle_stop);
  g_pipe->on("Connect",    handle_connect);
  g_pipe->on("Disconnect", handle_disconnect);
  g_pipe->start();
  logx::info(L"IPC server listening at \\\\.\\pipe\\vpnclient.pipe");

  // Ctrl+C → graceful stop
  SetConsoleCtrlHandler([](DWORD sig)->BOOL {
    if (sig == CTRL_C_EVENT || sig == CTRL_BREAK_EVENT || sig == CTRL_CLOSE_EVENT) {
      logx::info(L"Console signal -> stopping");
      g_running = false;
      return TRUE;
    }
    return FALSE;
  }, TRUE);

  while (g_running) { Sleep(100); }

  // teardown
  if (g_connected) handle_disconnect("{}");
  g_pipe->stop();
  logx::info(L"VPN Service stopped");
  return true;
}