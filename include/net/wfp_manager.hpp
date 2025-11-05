#pragma once
// Winsock must come before windows.h
#include <winsock2.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <windows.h>

// WFP types
#include <fwptypes.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace net {
  struct KillSwitchAllow {
    std::wstring             remoteHostOrIp;
    std::optional<uint16_t>  remotePort;
    int                      ipVersion = 4;             // 4 or 6
    int                      protocol  = IPPROTO_UDP;   // needs winsock headers
  };

  class WfpManager {
  public:
    bool init();
    bool enableKillSwitch(const std::vector<KillSwitchAllow>& allow);
    bool disableKillSwitch();

  private:
    bool addSublayer();
    bool addBlockAll();
    bool addAllowRules(const std::vector<KillSwitchAllow>& allow);
    void clearAll();

    HANDLE engine_ = nullptr;
    GUID   sublayerId_{};
    std::vector<UINT64> filterIds_;
  };
}