#pragma once
#include <string>
#include <vector>
namespace net {
  bool setDefaultRouteToInterface(const std::wstring& ifName);
  bool revertRoutes();
  bool setDnsServersForInterface(const std::wstring& ifName, const std::vector<std::wstring>& servers);
  bool revertDns(const std::wstring& ifName);
}
