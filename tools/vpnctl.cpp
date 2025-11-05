// tools/vpnctl.cpp
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>

static bool write_all(HANDLE h, const void* buf, DWORD len) {
  const BYTE* p = static_cast<const BYTE*>(buf);
  DWORD done = 0;
  while (done < len) {
    DWORD w = 0;
    if (!WriteFile(h, p + done, len - done, &w, nullptr)) return false;
    done += w;
  }
  return true;
}

static bool read_some(HANDLE h, std::string& out) {
  // read up to 64 KiB (single-response, our server replies once per request)
  std::vector<char> buf(64 * 1024);
  DWORD got = 0;
  if (!ReadFile(h, buf.data(), (DWORD)buf.size(), &got, nullptr)) return false;
  out.assign(buf.data(), buf.data() + got);
  return true;
}

static HANDLE open_pipe(const wchar_t* name, DWORD timeoutMs=2000) {
  if (!WaitNamedPipeW(name, timeoutMs)) return INVALID_HANDLE_VALUE;
  return CreateFileW(
    name, GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

static int send_request_print_reply(const std::wstring& pipename, const std::string& json) {
  HANDLE h = open_pipe(pipename.c_str());
  if (h == INVALID_HANDLE_VALUE) {
    std::cerr << "ERR: cannot open pipe\n";
    return 2;
  }

  // The server expects a single JSON blob and then closes. Send & read once.
  if (!write_all(h, json.data(), (DWORD)json.size())) {
    std::cerr << "ERR: write failed\n";
    CloseHandle(h);
    return 3;
  }
  // Flush so the server sees EOF after this message
  FlushFileBuffers(h);

  std::string resp;
  if (!read_some(h, resp)) {
    std::cerr << "ERR: read failed\n";
    CloseHandle(h);
    return 4;
  }

  std::cout << resp << std::endl;
  CloseHandle(h);
  return 0;
}

int wmain(int argc, wchar_t** wargv) {
  const std::wstring pipeName = L"\\\\.\\pipe\\vpnsvc";
  if (argc < 2) {
    std::wcerr << L"Usage:\n"
                  L"  vpnctl status\n"
                  L"  vpnctl connect <host:port> <pubkey> <privkey> [dns_csv]\n"
                  L"  vpnctl disconnect\n"
                  L"  vpnctl stop\n";
    return 1;
  }

  std::wstring cmd = wargv[1];
  if (cmd == L"status") {
    return send_request_print_reply(pipeName, R"({"method":"status"})");
  } else if (cmd == L"disconnect") {
    return send_request_print_reply(pipeName, R"({"method":"disconnect"})");
  } else if (cmd == L"stop") {
    return send_request_print_reply(pipeName, R"({"method":"stop"})");
  } else if (cmd == L"connect") {
    if (argc < 5) {
      std::wcerr << L"Usage: vpnctl connect <host:port> <pubkey> <privkey> [dns_csv]\n";
      return 1;
    }
    std::wstring whost = wargv[2], wpub = wargv[3], wpriv = wargv[4];
    std::wstring wdns = (argc >= 6) ? wargv[5] : L"";

    // naive wide->utf8 for these short values
    auto w2u = [](const std::wstring& ws) {
      int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
      std::string s(n, '\0');
      WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), s.data(), n, nullptr, nullptr);
      return s;
    };

    std::string host = w2u(whost);
    std::string pub  = w2u(wpub);
    std::string prv  = w2u(wpriv);
    std::string dns  = w2u(wdns);

    // very minimal JSON, server only needs method + fields
    std::string json = std::string("{\"method\":\"connect\",\"host\":\"") + host +
                       "\",\"pub\":\"" + pub + "\",\"priv\":\"" + prv + "\"";
    if (!dns.empty()) json += ",\"dns\":\"" + dns + "\"";
    json += "}";

    return send_request_print_reply(pipeName, json);
  }

  std::wcerr << L"Unknown command\n";
  return 1;
}