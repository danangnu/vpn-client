#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

static std::string call(const std::string& req) {
  HANDLE hPipe = CreateFileW(L"\\\\.\\pipe\\vpnclient.pipe",
      GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (hPipe == INVALID_HANDLE_VALUE) return "ERR: cannot open pipe";
  DWORD wr = 0; WriteFile(hPipe, req.data(), (DWORD)req.size(), &wr, nullptr);
  char buf[4096]; DWORD rd = 0;
  ReadFile(hPipe, buf, sizeof(buf), &rd, nullptr);
  CloseHandle(hPipe);
  return std::string(buf, rd);
}

int wmain(int argc, wchar_t** argv) {
  if (argc < 2) {
    std::wcout << L"Usage:\n"
                  L"  vpnctl status\n"
                  L"  vpnctl connect <endpoint> <peerPub> <privateKey> [dns1,dns2]\n"
                  L"  vpnctl disconnect\n"
                  L"  vpnctl stop\n";
    return 0;
  }

  std::wstring cmd = argv[1];

  if (cmd == L"status") {
    std::cout << call(R"({"method":"Status"})") << "\n";
  } else if (cmd == L"stop") {
    std::cout << call(R"({"method":"Stop"})") << "\n";
  } else if (cmd == L"disconnect") {
    std::cout << call(R"({"method":"Disconnect"})") << "\n";
  } else if (cmd == L"connect") {
    if (argc < 5) { std::wcout << L"connect requires: <endpoint> <peerPub> <privateKey> [dns1,dns2]\n"; return 1; }
    std::wstring wEndpoint = argv[2];
    std::wstring wPeer     = argv[3];
    std::wstring wPriv     = argv[4];
    std::wstring wDnsList  = (argc >= 6) ? argv[5] : L"";

    auto toNarrow = [](const std::wstring& ws){ return std::string(ws.begin(), ws.end()); };
    auto endpoint = toNarrow(wEndpoint);
    auto peer     = toNarrow(wPeer);
    auto priv     = toNarrow(wPriv);
    auto dns      = toNarrow(wDnsList);

    std::string dnsJson = "\"dns\":[";
    if (!dns.empty()) {
      size_t i=0; bool first=true;
      while (i < dns.size()) {
        auto j = dns.find(',', i);
        std::string item = (j == std::string::npos) ? dns.substr(i) : dns.substr(i, j-i);
        if (!first) dnsJson += ",";
        dnsJson += "\"" + item + "\"";
        first = false;
        if (j == std::string::npos) break;
        i = j+1;
      }
    }
    dnsJson += "]";

    std::string req = std::string(R"({"method":"Connect","params":{)") +
      "\"endpoint\":\"" + endpoint + "\","
      "\"peerPublicKey\":\"" + peer + "\","
      "\"privateKey\":\"" + priv + "\","
      "\"allowedIps\":\"0.0.0.0/0, ::/0\","
      + dnsJson +
    "}}";

    std::cout << call(req) << "\n";
  } else {
    std::wcout << L"Unknown cmd\n";
  }
  return 0;
}
