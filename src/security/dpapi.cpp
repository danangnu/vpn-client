#include "security/dpapi.hpp"
#include <stdexcept>
std::vector<uint8_t> dpapi::protect(const std::vector<uint8_t>& v, bool){ return v; }
std::vector<uint8_t> dpapi::unprotect(const std::vector<uint8_t>& v){ return v; }
