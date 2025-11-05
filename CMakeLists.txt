cmake_minimum_required(VERSION 3.25)
project(vpn-client LANGUAGES CXX)

# ===== Global settings =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Always build UNICODE on Windows, and keep headers lean.
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

# MSVC niceties: UTF-8 source, strict mode, warnings
if (MSVC)
  add_compile_options(/permissive- /W4 /Zc:__cplusplus /utf-8)
  # Optional: treat warnings as errors in CI
  # add_compile_options(/WX)
endif()

# Where headers live
include_directories(${CMAKE_SOURCE_DIR}/include)

# ===== Source lists =====
set(VPN_COMMON_SOURCES
    src/common/log.cpp
    src/net/wfp_manager.cpp
    src/net/route_dns.cpp
    src/ipc/pipe_server.cpp
)

set(SERVICE_SOURCES
    src/service/service.cpp
)

# The service has a WIN32 GUI-subsystem entry in service_main.cpp (wWinMain → StartServiceCtrlDispatcherW)
set(SERVICE_WINMAIN
    src/service/service_main.cpp
)

# Console/dev runner that calls into svc::run() directly – no SCM.
set(SERVICE_CONSOLE_MAIN
    src/service/console_main.cpp
)

# Tools
set(VPNCTL_SOURCES
    tools/vpnctl.cpp
)

set(SVCCTL_SOURCES
    tools/svcctl.cpp
)

# ===== Static library with shared code =====
add_library(vpn_common STATIC ${VPN_COMMON_SOURCES})
target_include_directories(vpn_common PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(vpn_common
  PUBLIC
    # Networking + WFP + DNS + services
    fwpuclnt      # WFP (Windows Filtering Platform)
    iphlpapi      # IP helper (routes, adapters, DNS)
    ws2_32        # Winsock
    advapi32      # Service / registry helpers if needed by common
)

# ===== Windows Service (real SCM-managed binary, WIN32 subsystem) =====
add_executable(vpn_service WIN32
  ${SERVICE_SOURCES}
  ${SERVICE_WINMAIN}
)
target_link_libraries(vpn_service PRIVATE vpn_common advapi32)

# Optional, makes the .exe friendlier to the debugger symbols
set_target_properties(vpn_service PROPERTIES
  OUTPUT_NAME "vpn_service"
)

# ===== Console variant for development =====
add_executable(vpn_service_console
  ${SERVICE_SOURCES}
  ${SERVICE_CONSOLE_MAIN}
)
target_link_libraries(vpn_service_console PRIVATE vpn_common)

set_target_properties(vpn_service_console PROPERTIES
  OUTPUT_NAME "vpn_service_console"
)

# ===== Tools =====
add_executable(vpnctl ${VPNCTL_SOURCES})
target_link_libraries(vpnctl PRIVATE vpn_common)

add_executable(svcctl ${SVCCTL_SOURCES})
target_link_libraries(svcctl PRIVATE vpn_common advapi32)

# ===== Nice MSVC folder organization (optional) =====
source_group(TREE ${CMAKE_SOURCE_DIR} FILES
  ${VPN_COMMON_SOURCES}
  ${SERVICE_SOURCES}
  ${SERVICE_WINMAIN}
  ${SERVICE_CONSOLE_MAIN}
  ${VPNCTL_SOURCES}
  ${SVCCTL_SOURCES}
)

# ===== Output dirs (multi-config friendly) =====
# CMake will place outputs under out/build/<config> with MSBuild / VS.
# If you want to force a flat bin/ & lib/ under the build dir, uncomment:
# foreach(t vpn_common vpn_service vpn_service_console vpnctl svcctl)
#   set_target_properties(${t} PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
#     ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
#     LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
#   )
# endforeach()