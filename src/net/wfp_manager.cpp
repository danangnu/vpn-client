#include <winsock2.h>
#include <ws2def.h>
#include <ws2ipdef.h>

#include "net/wfp_manager.hpp"
#include "common/log.hpp"

#include <fwptypes.h>
#include <fwpmu.h>
#include <vector>
#include <string.h>

#pragma comment(lib, "fwpuclnt.lib")

namespace net {

static GUID make_guid(uint32_t a) {
  GUID g{ a, 0xBEEF, 0xCAFE, {0xDE,0xAD,0xBE,0xEF,0x10,0x20,0x30,0x40} };
  return g;
}

bool WfpManager::init() {
  if (engine_) return true;
  DWORD s = FwpmEngineOpen0(nullptr, RPC_C_AUTHN_WINNT, nullptr, nullptr, &engine_);
  if (s != ERROR_SUCCESS) { logx::error(L"[WFP] FwpmEngineOpen failed"); engine_ = nullptr; return false; }
  sublayerId_ = make_guid(0xA11CE001);
  addSublayer();
  return true;
}

bool WfpManager::addSublayer() {
  FWPM_SUBLAYER0 sl{}; sl.subLayerKey = sublayerId_;
  sl.displayData.name = const_cast<wchar_t*>(L"VPN Client KillSwitch");
  sl.flags = 0; sl.weight = 0x100;
  DWORD s = FwpmSubLayerAdd0(engine_, &sl, nullptr);
  if (s != ERROR_SUCCESS && s != FWP_E_ALREADY_EXISTS) {
    logx::error(L"[WFP] SubLayerAdd failed");
    return false;
  }
  return true;
}

bool WfpManager::addBlockAll() {
  // Block outbound connects at ALE_AUTH_CONNECT for v4 and v6
  auto addBlock = [&](const GUID& layerKey) -> bool {
    FWPM_FILTER0 f{}; f.subLayerKey = sublayerId_;
    f.layerKey = layerKey;
    f.displayData.name = const_cast<wchar_t*>(L"VPN KillSwitch BlockAll");
    f.action.type = FWP_ACTION_BLOCK;
    f.weight.type = FWP_EMPTY; // default weight
    UINT64 id = 0;
    DWORD s = FwpmFilterAdd0(engine_, &f, nullptr, &id);
    if (s != ERROR_SUCCESS) return false;
    filterIds_.push_back(id);
    return true;
  };
  bool ok4 = addBlock(FWPM_LAYER_ALE_AUTH_CONNECT_V4);
  bool ok6 = addBlock(FWPM_LAYER_ALE_AUTH_CONNECT_V6);
  if (ok4 || ok6) logx::info(L"[WFP] BlockAll installed");
  return ok4 || ok6;
}

bool WfpManager::addAllowRules(const std::vector<KillSwitchAllow>& allow) {
  auto addAllow = [&](const KillSwitchAllow& a) -> bool {
    const GUID layer = (a.ipVersion == 6) ? FWPM_LAYER_ALE_AUTH_CONNECT_V6
                                          : FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    IN_ADDR  v4addr{}; IN6_ADDR v6addr{};
    FWP_BYTE_ARRAY16 v6bytes{};
    bool isIp = false;

    // Very simple IP parser (dotted v4 only here). Hostnames should be resolved externally.
    if (a.ipVersion == 4) {
      unsigned b1,b2,b3,b4;
      if (swscanf_s(a.remoteHostOrIp.c_str(), L"%u.%u.%u.%u", &b1,&b2,&b3,&b4) == 4) {
        v4addr.S_un.S_un_b = { (uint8_t)b1,(uint8_t)b2,(uint8_t)b3,(uint8_t)b4 };
        isIp = true;
      }
    }
    // (IPv6 parsing omitted for brevity in Day 4; can add later)

    FWPM_FILTER_CONDITION0 conds[3];
    UINT32 condCount = 0;

    // protocol condition
    conds[condCount].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
    conds[condCount].matchType = FWP_MATCH_EQUAL;
    conds[condCount].conditionValue.type = FWP_UINT8;
    conds[condCount].conditionValue.uint8 = (UINT8)a.protocol;
    condCount++;

    // remote address condition (only if IP parsed)
    if (isIp && a.ipVersion == 4) {
      conds[condCount].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
      conds[condCount].matchType = FWP_MATCH_EQUAL;
      conds[condCount].conditionValue.type = FWP_UINT32;
      conds[condCount].conditionValue.uint32 = v4addr.S_un.S_addr;
      condCount++;
    }
    // remote port condition (optional)
    FWP_RANGE0 portRange{};
    if (a.remotePort.has_value()) {
      conds[condCount].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
      conds[condCount].matchType = FWP_MATCH_EQUAL;
      conds[condCount].conditionValue.type = FWP_UINT16;
      conds[condCount].conditionValue.uint16 = *a.remotePort;
      condCount++;
    }

    FWPM_FILTER0 f{}; f.subLayerKey = sublayerId_;
    f.layerKey = layer;
    f.displayData.name = const_cast<wchar_t*>(L"VPN KillSwitch Allow");
    f.action.type = FWP_ACTION_PERMIT;
    f.filterCondition = conds;
    f.numFilterConditions = condCount;
    f.weight.type = FWP_EMPTY;
    UINT64 id = 0;
    DWORD s = FwpmFilterAdd0(engine_, &f, nullptr, &id);
    if (s != ERROR_SUCCESS) return false;
    filterIds_.push_back(id);
    return true;
  };

  bool any = false;
  for (auto& a : allow) any |= addAllow(a);
  if (any) logx::info(L"[WFP] Allow rules installed");
  return any;
}

void WfpManager::clearAll() {
  for (auto id : filterIds_) {
    FwpmFilterDeleteById0(engine_, id);
  }
  filterIds_.clear();
}

bool WfpManager::enableKillSwitch(const std::vector<KillSwitchAllow>& allow) {
  if (!engine_ && !init()) return false;
  clearAll();
  if (!addBlockAll()) return false;
  addAllowRules(allow);
  return true;
}

bool WfpManager::disableKillSwitch() {
  if (!engine_) return true;
  clearAll();
  logx::info(L"[WFP] KillSwitch removed");
  return true;
}

} // namespace net
