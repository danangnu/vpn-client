#pragma once
#include <string>
#include <optional>
#include <vector>   // <-- needed for std::vector

namespace net {
    struct KillSwitchAllow {
        std::wstring remoteHost;
        std::optional<unsigned short> remotePort;
        int ipVersion = 4;     // 4 or 6
        int protocol = 17;    // IPPROTO_UDP by default
    };

    class WfpManager {
    public:
        bool init() { return true; }
        bool enableKillSwitch(const std::wstring& tunnelLuid,
            const std::vector<KillSwitchAllow>& allow) {
            return true;
        }
        bool disableKillSwitch() { return true; }
    };

    bool setDefaultRouteToInterface(const std::wstring& ifName);
    bool revertRoutes();
    bool setDnsServersForInterface(const std::wstring& ifName,
        const std::vector<std::wstring>& servers);
    bool revertDns(const std::wstring& ifName);
}
