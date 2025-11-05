cmake_minimum_required(VERSION 3.24)
project(VpnClient LANGUAGES CXX VERSION 0.1.0)

# --- C++ / MSVC config -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  add_compile_options(
    /W4             # high warnings
    /permissive-    # strict mode
    /Zc:preprocessor
    /EHsc           # C++ exceptions
    /utf-8          # source files are UTF-8
    /DUNICODE /D_UNICODE
    /DWIN32_LEAN_AND_MEAN
    /D_CRT_SECURE_NO_WARNINGS
  )
endif()

# Place binaries in a clean tree: build/<config>/
# (CMake Tools usually sets this; keeping for consistency)
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# --- Includes ----------------------------------------------------------------
# We keep headers in ./include
include_directories(${CMAKE_SOURCE_DIR}/include)

# --- Common library (logging, DPAPI, IPC, WFP, routing, VPN handlers) --------
set(VPN_COMMON_SOURCES
  src/common/log.cpp
  src/security/dpapi.cpp
  src/ipc/pipe_server.cpp
  src/net/wfp_manager.cpp
  src/net/route_dns.cpp
  src/vpn/wireguard_handler.cpp
  src/vpn/openvpn_handler.cpp
)

add_library(vpn_common ${VPN_COMMON_SOURCES})
target_include_directories(vpn_common PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Windows libs:
#  - crypt32   : DPAPI (CryptProtectData etc.)
#  - fwpuclnt  : WFP user-mode API
#  - iphlpapi  : routing/DNS helpers
#  - shell32   : SHGetKnownFolderPath used in resilient logger
target_link_libraries(vpn_common
  PRIVATE
    crypt32.lib
    fwpuclnt.lib
    iphlpapi.lib
    shell32.lib
)

# --- Windows Service executable ----------------------------------------------
add_executable(vpn_service
  src/service/service.cpp
  src/service/service_main.cpp
)

# advapi32: Service Control Manager APIs
target_link_libraries(vpn_service PRIVATE vpn_common advapi32.lib)

# --- Optional: console-only entry (dev/debug convenience) --------------------
# This lets you run the service loop without the SCM.
if (EXISTS "${CMAKE_SOURCE_DIR}/src/service/console_main.cpp")
      add_executable(vpn_service_console
      src/service/console_main.cpp
      src/service/service.cpp        # <-- add this so svc::run() is linked
    )
    target_link_libraries(vpn_service_console PRIVATE vpn_common)
endif()

# --- Tools -------------------------------------------------------------------
# Service installer/remover helper (install/uninstall/start/stop via SCM later if extended)
add_executable(svcctl tools/svcctl.cpp)
target_link_libraries(svcctl PRIVATE advapi32.lib)

# Tiny pipe test client
if (EXISTS "${CMAKE_SOURCE_DIR}/tools/vpnctl.cpp")
  add_executable(vpnctl tools/vpnctl.cpp)
endif()

# --- Nice-to-have: show a summary -------------------------------------------
message(STATUS "==== Build Summary ====")
message(STATUS "Generator:       ${CMAKE_GENERATOR}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "Source dir:      ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary dir:      ${CMAKE_BINARY_DIR}")
message(STATUS "======================")
