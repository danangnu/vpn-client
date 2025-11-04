#include "service/service.hpp"
#include "common/log.hpp"
#include <Windows.h>
#include <string>
#include <vector>

SERVICE_STATUS        g_status{};
SERVICE_STATUS_HANDLE g_statusHandle = nullptr;

static void SetStatus(DWORD state, DWORD win32Exit = NO_ERROR) {
  if (!g_statusHandle) return; // only valid when running as a real service
  g_status.dwCurrentState = state;
  g_status.dwWin32ExitCode = win32Exit;
  g_status.dwControlsAccepted = (state == SERVICE_RUNNING) ? SERVICE_ACCEPT_STOP : 0;
  SetServiceStatus(g_statusHandle, &g_status);
}

static void WINAPI ServiceCtrlHandler(DWORD ctrl) {
  if (ctrl == SERVICE_CONTROL_STOP) SetStatus(SERVICE_STOP_PENDING);
  SetStatus(SERVICE_STOPPED);
}

static void WINAPI ServiceMain(DWORD, LPWSTR*) {
  g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  g_statusHandle = RegisterServiceCtrlHandlerW(svc::kServiceName, ServiceCtrlHandler);
  SetStatus(SERVICE_START_PENDING);

  logx::init(L"C:\\ProgramData\\VpnClient\\vpn-service.log");
  if (!svc::run()) {
    SetStatus(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR);
    return;
  }
  SetStatus(SERVICE_RUNNING);
  SetStatus(SERVICE_STOPPED);
}

static bool has_arg(int argc, wchar_t** argv, const wchar_t* flag) {
  for (int i = 1; i < argc; ++i) if (0 == _wcsicmp(argv[i], flag)) return true;
  return false;
}

int wmain(int argc, wchar_t** argv) {
  // Run as console unless explicitly told to be a service
  if (!has_arg(argc, argv, L"--service")) {
    logx::init(L"");            // <- also use fallback here
    svc::run();
    return 0;
  }

  // Real Windows Service mode (requires SCM).
  SERVICE_TABLE_ENTRYW table[] = {
    { const_cast<LPWSTR>(svc::kServiceName), ServiceMain },
    { nullptr, nullptr }
  };
  if (!StartServiceCtrlDispatcherW(table)) {
    // Fallback to console if SCM attach fails
    logx::init(L"C:\\ProgramData\\VpnClient\\vpn-service.log");
    svc::run();
  }
  return 0;
}
