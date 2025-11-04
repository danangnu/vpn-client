#include "ipc/pipe_server.hpp"
#include "common/log.hpp"
#include <Windows.h>
#include <vector>

using namespace ipc;

PipeServer::PipeServer(const std::wstring& name)
  : pipeName_(L"\\\\.\\pipe\\" + name) {}

PipeServer::~PipeServer() { stop(); }

void PipeServer::on(const std::string& method, Handler h) {
  handlers_[method] = std::move(h);
}

static std::string dispatch(std::unordered_map<std::string, Handler>& handlers, const std::string& req) {
  // Very naive JSON method parser: {"method":"X"...}
  auto pos = req.find("\"method\"");
  if (pos == std::string::npos) return "{\"error\":\"bad request\"}";
  auto q1 = req.find('"', pos + 8);
  if (q1 == std::string::npos) return "{\"error\":\"bad request\"}";
  auto q2 = req.find('"', q1 + 1);
  if (q2 == std::string::npos) return "{\"error\":\"bad request\"}";
  std::string method = req.substr(q1 + 1, q2 - q1 - 1);

  auto it = handlers.find(method);
  if (it == handlers.end()) return "{\"error\":\"unknown method\"}";
  return it->second(req);
}

void PipeServer::start() {
  running_ = true;
  th_ = std::thread([this] {
    while (running_) {
      HANDLE hPipe = CreateNamedPipeW(
        pipeName_.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        8, 64 * 1024, 64 * 1024, 0, nullptr);

      if (hPipe == INVALID_HANDLE_VALUE) {
        logx::error(L"CreateNamedPipe failed");
        break;
      }

      BOOL ok = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
      if (!ok) { CloseHandle(hPipe); continue; }

      std::vector<char> buf(64 * 1024);
      DWORD rd = 0;
      if (!ReadFile(hPipe, buf.data(), (DWORD)buf.size(), &rd, nullptr)) {
        DisconnectNamedPipe(hPipe); CloseHandle(hPipe); continue;
      }

      std::string req(buf.data(), rd);
      std::string resp = dispatch(handlers_, req);

      DWORD wr = 0;
      WriteFile(hPipe, resp.data(), (DWORD)resp.size(), &wr, nullptr);
      FlushFileBuffers(hPipe);
      DisconnectNamedPipe(hPipe);
      CloseHandle(hPipe);
    }
  });
}

void PipeServer::stop() {
  if (!running_) return;
  running_ = false;

  // Nudge the server once so ConnectNamedPipe unblocks.
  HANDLE h = CreateFileW(pipeName_.c_str(), GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (h != INVALID_HANDLE_VALUE) { CloseHandle(h); }

  if (th_.joinable()) th_.join();
}
