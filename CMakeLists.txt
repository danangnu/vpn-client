cmake_minimum_required(VERSION 3.24)
project(VpnClientService LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable MSVC warnings-as-errors for our sources (adjust as needed)
if (MSVC)
  add_compile_options(/W4 /permissive- /Zc:preprocessor)
endif()

# nlohmann/json single header (drop a copy to include/common/json.hpp)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(vpn_common
  src/common/log.cpp
  src/security/dpapi.cpp
  src/ipc/pipe_server.cpp
  src/net/wfp_manager.cpp
  src/net/route_dns.cpp
  src/vpn/wireguard_handler.cpp
  src/vpn/openvpn_handler.cpp
)
target_include_directories(vpn_common PUBLIC include)
target_link_libraries(vpn_common
  # Windows libs
  crypt32.lib      # DPAPI
  fwpuclnt.lib     # WFP user-mode API
  rpcrt4.lib       # (Named pipes via Win32; rpcrt4 if you later use RPC)
  iphlpapi.lib     # routing/DNS helpers
)

add_executable(vpn_service
  src/service/service.cpp
  src/service/service_main.cpp
)
target_link_libraries(vpn_service vpn_common
  advapi32.lib     # Windows Service APIs
)

add_executable(svcctl tools/svcctl.cpp)
target_link_libraries(svcctl advapi32.lib)
