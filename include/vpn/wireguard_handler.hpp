#pragma once
#include <string>
#include <optional>
namespace vpn {
  struct WgConfig { std::string privateKey, publicKey, peerPublicKey, endpoint, allowedIps; std::optional<int> mtu; };
  class WireGuardHandler { public: bool createInterface(const std::wstring&){return true;} bool applyConfig(const WgConfig&){return true;} bool up(){return true;} bool down(){return true;} };
  class OpenVpnHandler { public: bool connect(){return true;} bool disconnect(){return true;} };
}
