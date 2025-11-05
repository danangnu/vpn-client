#pragma once
#include <string>
#include <vector>
#include <optional>

namespace net {
  struct RouteSnapshot {
    // what default routes existed before we touched them
    // (we store the raw route entries as text for simplicity)
    std::vector<std::wstring> v4Defaults;
    std::vector<std::wstring> v6Defaults;
    // previous DNS for the interface (string IPs)
    std::vector<std::wstring> dnsPrev;
    std::wstring ifAlias;
  };

  // Capture current defaults + DNS of the chosen interface alias (e.g., "vg0")
  std::optional<RouteSnapshot> snapshot(const std::wstring& ifAlias);

  // Route: set default route via interface alias (best-effort)
  bool setDefaultRouteToInterface(const std::wstring& ifAlias);

  // DNS: set specific servers on the alias (PowerShell cmdlets)
  bool setDnsServersForInterface(const std::wstring& ifAlias,
                                 const std::vector<std::wstring>& servers);

  // Revert both routes and DNS using the snapshot info
  bool revert(const RouteSnapshot& snap);
}
