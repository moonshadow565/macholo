#pragma once
#include <string_view>

namespace lib {
  extern void* resolve(std::string_view name);
  extern void* resolve_import(std::string_view name);
}
