#pragma once
#include "common.hpp"
#include "macho.hpp"

namespace loader {
    struct Image {
        std::string path;
        void* address;
        uintptr_t slide;
        // load() got one mapping per segment (rather than the flat whole-slice fallback),
        // so protect() may hand out the real per-segment protections.
        bool segments_mapped;
        // Anonymous shared file holding this image's slice; the segments are mapped
        // MAP_SHARED from it, which is what makes a second, aliasing mapping of the
        // same pages possible (mach_vm_remap). -1 when the flat fallback was used.
        int backing_fd;
        // Size of that backing file, i.e. the image's whole VM span.
        size_t backing_size;
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

        // Drop each segment to its real protections. Must run after bind()/init(), which
        // are what need __DATA_CONST (and everything else) temporarily writable.
        void protect();

        int run(int argc, char** argv, char** env);
    };
}
