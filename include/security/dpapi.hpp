#pragma once
#include <vector>
namespace dpapi { std::vector<uint8_t> protect(const std::vector<uint8_t>&, bool); std::vector<uint8_t> unprotect(const std::vector<uint8_t>&); }
