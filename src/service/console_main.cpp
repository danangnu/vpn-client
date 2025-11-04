#include "service/service.hpp"
#include "common/log.hpp"
#include <crtdbg.h>    // debug report control
#include <cstdlib>
#include <iostream>

int wmain() {
  // Silence CRT abort popups in debug; write to stderr instead.
#if _DEBUG
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
  // Don’t prefer ProgramData in console mode — let logger pick a writable place.
  logx::init(L"");  // <- empty = use fallback chain
  std::wcout << L"[console] vpn_service_console starting...\n";
  return svc::run() ? 0 : 1;
}
