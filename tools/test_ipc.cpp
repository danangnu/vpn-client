#include <Windows.h>
#include <iostream>
#include <string>

static std::string call(const std::string& req) {
  HANDLE hPipe = CreateFileW(L"\\\\.\\pipe\\vpnclient.pipe",
      GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (hPipe == INVALID_HANDLE_VALUE) return "ERR: cannot open pipe";
  DWORD wr = 0; WriteFile(hPipe, req.data(), (DWORD)req.size(), &wr, nullptr);
  char buf[1024]; DWORD rd = 0;
  ReadFile(hPipe, buf, sizeof(buf), &rd, nullptr);
  CloseHandle(hPipe);
  return std::string(buf, rd);
}

int wmain(int argc, wchar_t**) {
  std::cout << "Status: " << call(R"({"method":"Status"})") << "\n";
  return 0;
}