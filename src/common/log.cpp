#include "common/log.hpp"
#include <windows.h>
#include <cstdio>
#include <mutex>
#include <ctime>
#include <cwchar>

namespace {
  std::mutex g_mu;
  FILE* g_file = nullptr;

  std::wstring timestamp_utc() {
    SYSTEMTIME st{};
    GetSystemTime(&st);
    wchar_t buf[64];
    swprintf(buf, 64, L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
  }

  void write_line(const wchar_t* level, const std::wstring& msg) {
    std::lock_guard<std::mutex> lk(g_mu);
    const std::wstring line = L"[" + timestamp_utc() + L"][" + level + L"] " + msg + L"\r\n";

    // 1) Always emit to debugger (no stdio involved)
    OutputDebugStringW(line.c_str());

    // 2) If a file is open, write as wide text (file stream is wide-oriented)
    if (g_file) {
      // g_file is created with ccs=UTF-8 so fwprintf is valid wide I/O
      fwprintf(g_file, L"%s", line.c_str());
      fflush(g_file);
    }
  }
}

namespace logx {

void init(const std::wstring& filePath) {
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_file) return;

  if (!filePath.empty()) {
    // Open as a wide stream in UTF-8 mode; no stdout/stderr touching
    g_file = _wfopen(filePath.c_str(), L"a+, ccs=UTF-8");
    if (g_file) {
      // Optional header
      fwprintf(g_file, L"==== log start %s ====\r\n", timestamp_utc().c_str());
      fflush(g_file);
    }
  }
}

void shutdown() {
  std::lock_guard<std::mutex> lk(g_mu);
  if (g_file) {
    fwprintf(g_file, L"==== log end %s ====\r\n", timestamp_utc().c_str());
    fclose(g_file);
    g_file = nullptr;
  }
}

void info(const std::wstring& msg)  { write_line(L"INFO",  msg); }
void error(const std::wstring& msg) { write_line(L"ERROR", msg); }

} // namespace logx
