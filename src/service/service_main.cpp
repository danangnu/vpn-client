// src/service/service_main.cpp
#include <windows.h>
#include <string>
#include "service/service.hpp"   // declares: namespace svc { bool run(); void stop(); }
#include "common/log.hpp"        // logx::init/info/error (already linked in vpn_common)

// ---- Service name (must match your installer / svcctl) ----
static constexpr wchar_t kServiceName[] = L"vpn_service";

// ---- Globals used for status reporting to the SCM ----
static SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
static SERVICE_STATUS        g_status       = {};

// Helper to set and report service status to the SCM
static void SetServiceStatusToSCM(
    DWORD state,
    DWORD win32ExitCode = NO_ERROR,
    DWORD waitHintMs    = 0,
    DWORD checkPoint    = 0)
{
    g_status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwCurrentState            = state;
    g_status.dwWin32ExitCode           = win32ExitCode;
    g_status.dwServiceSpecificExitCode = 0;
    g_status.dwWaitHint                = waitHintMs;

    // Accept controls only when running
    g_status.dwControlsAccepted = 0;
    if (state == SERVICE_RUNNING) {
        g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    } else if (state == SERVICE_STOP_PENDING || state == SERVICE_START_PENDING) {
        // No control accepted while pending
        g_status.dwControlsAccepted = 0;
    }

    // Only increment checkpoint for pending states
    if (state == SERVICE_START_PENDING || state == SERVICE_STOP_PENDING) {
        static DWORD s_cp = 1;
        g_status.dwCheckPoint = checkPoint ? checkPoint : s_cp++;
    } else {
        g_status.dwCheckPoint = 0;
    }

    if (g_statusHandle) {
        ::SetServiceStatus(g_statusHandle, &g_status);
    }
}

// Control handler: stop/shutdown → ask service code to stop
static DWORD WINAPI ServiceCtrlHandlerEx(DWORD control, DWORD /*eventType*/, LPVOID /*eventData*/, LPVOID /*context*/)
{
    switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        SetServiceStatusToSCM(SERVICE_STOP_PENDING, NO_ERROR, 2000);
        svc::stop();                  // your implementation signals the stop event
        return NO_ERROR;
    default:
        return NO_ERROR;
    }
}

// The ServiceMain entry that the SCM calls
namespace svc {
void WINAPI ServiceMain(DWORD /*argc*/, LPWSTR* /*argv*/)
{
    // Register handler first
    g_statusHandle = ::RegisterServiceCtrlHandlerExW(kServiceName, ServiceCtrlHandlerEx, nullptr);
    if (!g_statusHandle) {
        return; // cannot report status; nothing else to do
    }

    SetServiceStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 2000);

    // Initialize logging to a default sink (adjust path if your logger expects one)
    logx::init(L"vpn_service");

    // Now running: report to SCM
    SetServiceStatusToSCM(SERVICE_RUNNING);

    // Block in your service body until stop() is called
    const bool ok = svc::run();

    // Report final state
    SetServiceStatusToSCM(SERVICE_STOPPED, ok ? NO_ERROR : ERROR_SERVICE_SPECIFIC_ERROR);
}
} // namespace svc

// GUI subsystem entry point required for WIN32 subsystem targets.
// Starts the service control dispatcher which calls svc::ServiceMain.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    static SERVICE_TABLE_ENTRYW table[] = {
        { const_cast<LPWSTR>(kServiceName), &svc::ServiceMain },
        { nullptr, nullptr }
    };

    if (!::StartServiceCtrlDispatcherW(table)) {
        // If launched manually (not by SCM), return error so callers see failure.
        // (Optional) Write to debugger so you can see why:
        wchar_t buf[128];
        wsprintfW(buf, L"StartServiceCtrlDispatcherW failed: %lu\n", GetLastError());
        ::OutputDebugStringW(buf);
        return static_cast<int>(GetLastError());
    }
    return 0;
}