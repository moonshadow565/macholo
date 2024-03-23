#pragma once
#include "loader.hpp"

namespace objc {
    extern void fixup_image(loader::Image* image);
    extern bool is_objc_name(std::string_view name);
}
