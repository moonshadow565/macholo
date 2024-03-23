#pragma once
#include "common.hpp"
#include "macho.hpp"

namespace loader {
    struct Image {
        std::string path;
        void* address;
        uintptr_t slide;
        macho::Object object;

        template<typename T>
        slice<T> find_data_section(std::string_view name) {
            if (auto seg = object.data_segment()) {
                for (auto const& sec : seg->sections) {
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
