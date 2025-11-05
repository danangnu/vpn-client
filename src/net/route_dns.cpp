#include "net/route_dns.hpp"
#include "common/log.hpp"
#include <Windows.h>
#include <iphlpapi.h>
#include <cstdio>
#include <string>
#include <vector>
#include <optional>

#pragma comment(lib, "iphlpapi.lib")

namespace {

bool runPowershell(const std::wstring& script, std::wstring* out = nullptr) {
  // powershell.exe -NoProfile -NonInteractive -Command "<script>"
  std::wstring cmd = L"powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command ";
  cmd += L"\"" + script + L"\"";

  SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
  HANDLE rd = nullptr, wr = nullptr;
  if (!CreatePipe(&rd, &wr, &sa, 0)) return false;
  SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si{ sizeof(si) };
  PROCESS_INFORMATION pi{};
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = wr; si.hStdError = wr;

  std::wstring cmdline = cmd;
  BOOL ok = CreateProcessW(
    nullptr, cmdline.data(), nullptr, nullptr, TRUE,
    CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

  CloseHandle(wr);
  if (!ok) { CloseHandle(rd); return false; }

  std::wstring buffer;
  wchar_t chunk[1024];
  DWORD read = 0;
  while (ReadFile(rd, chunk, sizeof(chunk)-sizeof(wchar_t), &read, nullptr) && read) {
    chunk[read / sizeof(wchar_t)] = L'\0';
    buffer.append(chunk, chunk + (read / sizeof(wchar_t)));
  }
  CloseHandle(rd);
  WaitForSingleObject(pi.hProcess, 60000);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  if (out) *out = buffer;
  return true;
}

std::wstring esc(const std::wstring& s) {
  std::wstring r = s;
  for (auto& ch : r) if (ch == L'"') ch = L'\'';
  return r;
}

} // namespace

namespace net {

static std::vector<std::wstring> parseDnsList(const std::wstring& stdoutText) {
  // We expect one IP per line from our PS call
  std::vector<std::wstring> r;
  std::wstring cur;
  for (wchar_t c : stdoutText) {
    if (c == L'\r') continue;
    if (c == L'\n') {
      if (!cur.empty()) r.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) r.push_back(cur);
  return r;
}

std::optional<RouteSnapshot> snapshot(const std::wstring& ifAlias) {
  RouteSnapshot snap; snap.ifAlias = ifAlias;

  // DNS snapshot (PowerShell)
  // Get server addresses as newline-separated IPs
  std::wstring out;
  std::wstring ps = L"(Get-DnsClientServerAddress -InterfaceAlias \"" + esc(ifAlias) +
                    L"\" -AddressFamily IPv4).ServerAddresses; "  // IPv4
                    L"(Get-DnsClientServerAddress -InterfaceAlias \"" + esc(ifAlias) +
                    L"\" -AddressFamily IPv6).ServerAddresses";
  if (runPowershell(ps, &out)) {
    snap.dnsPrev = parseDnsList(out);
  }

  // Route snapshot (keep defaults as strings using 'route print' to avoid IP Helper complexity)
  std::wstring out2;
  if (runPowershell(L"route print -4 | Out-String", &out2)) {
    // store the whole -4 table (lightweight; we won't parse today)
    snap.v4Defaults.push_back(out2);
  }
  if (runPowershell(L"route print -6 | Out-String", &out2)) {
    snap.v6Defaults.push_back(out2);
  }

  return snap;
}

bool setDefaultRouteToInterface(const std::wstring& ifAlias) {
  // Best-effort: set strong host default via interface alias using PowerShell cmdlets
  // (Production: use CreateIpForwardEntry2 with interface LUID.)
  std::wstring ps4 = L"Get-NetIPInterface -InterfaceAlias \"" + esc(ifAlias) +
                     L"\" -AddressFamily IPv4 | Set-NetIPInterface -InterfaceMetric 1";
  std::wstring ps6 = L"Get-NetIPInterface -InterfaceAlias \"" + esc(ifAlias) +
                     L"\" -AddressFamily IPv6 | Set-NetIPInterface -InterfaceMetric 1";
  bool ok4 = runPowershell(ps4);
  bool ok6 = runPowershell(ps6);
  logx::info(L"[ROUTE] default preference moved to " + ifAlias);
  return ok4 || ok6;
}

bool setDnsServersForInterface(const std::wstring& ifAlias,
                               const std::vector<std::wstring>& servers) {
  // Use Set-DnsClientServerAddress. If empty => automatic.
  bool ok = true;
  if (servers.empty()) {
    std::wstring ps = L"Set-DnsClientServerAddress -InterfaceAlias \"" + esc(ifAlias) + L"\" -ResetServerAddresses";
    ok = runPowershell(ps);
  } else {
    std::wstring list = L"@(";
    for (size_t i=0;i<servers.size();++i) {
      if (i) list += L",";
      list += L"\"" + esc(servers[i]) + L"\"";
    }
    list += L")";
    std::wstring ps = L"Set-DnsClientServerAddress -InterfaceAlias \"" + esc(ifAlias) + L"\" -ServerAddresses " + list;
    ok = runPowershell(ps);
  }
  if (ok) logx::info(L"[DNS] servers set on " + ifAlias);
  return ok;
}

bool revert(const RouteSnapshot& snap) {
  // DNS revert
  bool ok = true;
  ok &= setDnsServersForInterface(snap.ifAlias, snap.dnsPrev);

  // Route revert (simple metric relax so Windows prefers original)
  std::wstring ps4 = L"Get-NetIPInterface -InterfaceAlias \"" + esc(snap.ifAlias) +
                     L"\" -AddressFamily IPv4 | Set-NetIPInterface -InterfaceMetric 25";
  std::wstring ps6 = L"Get-NetIPInterface -InterfaceAlias \"" + esc(snap.ifAlias) +
                     L"\" -AddressFamily IPv6 | Set-NetIPInterface -InterfaceMetric 25";
  runPowershell(ps4);
  runPowershell(ps6);
  logx::info(L"[ROUTE] reverted metric preference for " + snap.ifAlias);
  return ok;
}

} // namespace net
