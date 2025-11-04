#include "service/service.hpp"
#include <Windows.h>
#include <thread>
#include <atomic>
static std::atomic<bool> running = true;
bool svc::run() {
  for (int i=0;i<5 && running;i++) { Sleep(200); }
  return true;
}
