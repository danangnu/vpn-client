#include "net/route_dns.hpp"
namespace net {
  bool setDefaultRouteToInterface(const std::wstring&){ return true; }
  bool revertRoutes(){ return true; }
  bool setDnsServersForInterface(const std::wstring&, const std::vector<std::wstring>&){ return true; }
  bool revertDns(const std::wstring&){ return true; }
}
