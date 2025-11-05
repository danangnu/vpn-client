#include <Windows.h>
#include <cstdio>

namespace svc { bool run(); }

int wmain() {
  std::puts("[console] vpn_service_console starting...");
  const bool ok = svc::run();
  std::printf("[console] svc::run() returned %s\n", ok ? "true" : "false");
  return ok ? 0 : 1;
}
