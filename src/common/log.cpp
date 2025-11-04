#include "common/log.hpp"
#include <Windows.h>
#include <shlobj.h>    // SHGetKnownFolderPath
#include <filesystem>
#include <fstream>
#include <mutex>

#pragma comment(lib, "Shell32.lib")

namespace {
  std::wofstream g_out;
  std::mutex g_mu;

  inline void safe_open(const std::wstring& path) {
    try {
      std::filesystem::path p(path);
      if (!p.parent_path().empty()) {
        std::error_code ec; std::filesystem::create_directories(p.parent_path(), ec);
      }
      g_out.open(path, std::ios::app);
    } catch (...) { /* swallow */ }
  }

  inline std::wstring knownFolder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) {
      std::wstring s = p; CoTaskMemFree(p); return s;
    }
    return L"";
  }

  inline void write_line(const wchar_t* lvl, const std::wstring& m) {
    SYSTEMTIME st{}; GetLocalTime(&st);
    wchar_t ts[64]{};
    swprintf(ts, 64, L"%04u-%02u-%02u %02u:%02u:%02u",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::scoped_lock lk(g_mu);
    if (g_out.is_open()) g_out << L"[" << ts << L"][" << lvl << L"] " << m << L"\n";
  }
}

void logx::init(const std::wstring& preferredPath) {
  // Try preferred path (if any), otherwise choose a writable fallback.
  if (!preferredPath.empty()) safe_open(preferredPath);

  if (!g_out.is_open()) {
    // ProgramData\VpnClient
    auto pd = knownFolder(FOLDERID_ProgramData);
    if (!pd.empty()) safe_open((std::filesystem::path(pd) / L"VpnClient" / L"vpn-service.log").wstring());
  }
  if (!g_out.is_open()) {
    // LocalAppData\VpnClient
    auto lad = knownFolder(FOLDERID_LocalAppData);
    if (!lad.empty()) safe_open((std::filesystem::path(lad) / L"VpnClient" / L"vpn-service.log").wstring());
  }
  if (!g_out.is_open()) {
    // Current working dir
    safe_open(L".\\vpn-service.log");
  }

  write_line(L"INFO", L"logging initialized");
}

void logx::info(const std::wstring& m)  { write_line(L"INFO",  m); }
void logx::error(const std::wstring& m) { write_line(L"ERROR", m); }
