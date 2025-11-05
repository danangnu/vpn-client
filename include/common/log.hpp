#pragma once
#include <string>

namespace logx {

// Call once at process start. If path is empty, we still log to OutputDebugStringW.
void init(const std::wstring& filePath);

// Optional: rotate/close on shutdown (safe to call multiple times).
void shutdown();

// Simple wide logging APIs (no stdout/stderr usage).
void info(const std::wstring& msg);
void error(const std::wstring& msg);

} // namespace logx
