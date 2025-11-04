#pragma once
#include <functional>
#include <string>
namespace ipc {
  using Handler = std::function<std::string(const std::string&)>;
  class PipeServer { public: explicit PipeServer(const std::wstring&){} void start(){} void stop(){} void on(const std::string&, Handler){} };
}
