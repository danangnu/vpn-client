#include "service/service.hpp"
#include "common/log.hpp"
#include <Windows.h>

SERVICE_STATUS g_status{}; SERVICE_STATUS_HANDLE g_sh = nullptr;
static void SetStatus(DWORD s){ g_status.dwCurrentState = s; SetServiceStatus(g_sh, &g_status); }
void WINAPI ServiceCtrlHandler(DWORD c){ if(c==SERVICE_CONTROL_STOP) SetStatus(SERVICE_STOPPED); }
void WINAPI ServiceMain(DWORD, LPWSTR*) {
  g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  g_sh = RegisterServiceCtrlHandlerW(svc::kServiceName, ServiceCtrlHandler);
  SetStatus(SERVICE_RUNNING);
  svc::run();
  SetStatus(SERVICE_STOPPED);
}
int wmain() {
  SERVICE_TABLE_ENTRYW t[] = { { const_cast<LPWSTR>(svc::kServiceName), ServiceMain }, {nullptr,nullptr} };
  if(!StartServiceCtrlDispatcherW(t)) { svc::run(); }
  return 0;
}
