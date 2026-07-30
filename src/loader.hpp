#pragma once
#include "common.hpp"
#include "macho.hpp"

namespace loader {
    struct Image {
        std::string path;
        void* address;
        uintptr_t slide;
        macho::Object object;

        // NOTE: searches all segments — modern binaries split sections like __mod_init_func,
        // __objc_classlist and __cfstring into __DATA_CONST.
        template<typename T>
        slice<T> find_section(std::string_view name) {
            for (auto const& seg : object.segments) {
                for (auto const& sec : seg.sections) {
                    if (sec.name != name) {
                        continue;
                    }
                    return { (T*)(sec.addr + slide), sec.size / sizeof(T) };
                }
            }
            return {};
        }

        static Image* load(char const* name);

        static Image* main;

        void bind();

        void init();

        int run(int argc, char** argv, char** env);
    };
}
