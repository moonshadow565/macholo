#include <sys/mman.h>

#include <cstdio>
#include <map>
#include <set>
#include <string>

#include "common.hpp"
#include "macho.hpp"

// Bind ordinals: 1-based dylib index; <= 0 are special (self / main-executable / flat / weak lookup).
static std::string ordinal_name(macho::Object const& object, int64_t ordinal) {
  if (ordinal >= 1 && (size_t)ordinal <= object.dylibs.size()) {
    auto const& dylib = object.dylibs[ordinal - 1];
    auto const slash = dylib.name.rfind('/');
    return slash == std::string::npos ? dylib.name : dylib.name.substr(slash + 1);
  }
  return "special(" + std::to_string(ordinal) + ")";
}

// Standalone Mach-O inspector: parses like the loader does (FAT slice + Object::read) but
// executes nothing. Seed of the `--audit` tooling from docs/PLAN.md phase 0.
int main(int argc, char** argv) {
  bool print_syms = false;
  int argi = 1;
  if (argi < argc && std::string_view(argv[argi]) == "--syms") {
    print_syms = true;
    ++argi;
  }
  if (argi == argc) {
    fprintf(stderr, "usage: %s [--syms] <macho-file>...\n", argv[0]);
    return 1;
  }

  for (; argi != argc; ++argi) {
    auto const path = argv[argi];
    unique_fd fd(::open(path, O_RDONLY));
    assert_that(fd.get() != -1);
    size_t const size = fd.size();
    assert_that(size != 0);
    void* file = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd.get(), 0);
    assert_that(file != nullptr && (intptr_t)file != -1);

    auto const sl = macho::find_slice(file, size);
    auto const object = macho::Object::read((char*)file + sl.offset);

    printf("%s:\n", path);
    printf("  slice: offset=0x%zx size=0x%zx%s\n", sl.offset, sl.size, sl.offset || sl.size != size ? " (FAT)" : " (thin)");
    for (auto const& seg : object.segments) {
      printf("  segment %-16s vmaddr=0x%09lx vmsize=0x%09lx\n", seg.name.c_str(), seg.vmaddr, seg.vmsize);
      for (auto const& sec : seg.sections) {
        printf("    %-24s addr=0x%09lx size=0x%08lx type=0x%02x\n", sec.name.c_str(), sec.addr, sec.size, sec.type);
      }
    }
    for (auto const& rpath : object.rpaths) printf("  rpath %s\n", rpath.c_str());
    for (auto const& dylib : object.dylibs) printf("  dylib %s%s\n", dylib.name.c_str(), dylib.weak ? " (weak)" : "");
    if (object.entry) printf("  entry: offset=0x%lx\n", object.entry->offset);
    printf("  rebase=%zu bind=%zu lazy=%zu weak=%zu exports=%zu tlv_start=0x%zx tlv_size=0x%zx\n",
           object.rebase.size(), object.bind.size(), object.bind_lazy.size(), object.bind_weak.size(),
           object.exports.size(), object.tlv_start, object.tlv_size);

    std::map<std::string, std::set<std::string>> by_dylib;
    for (auto const& b : object.bind) by_dylib[ordinal_name(object, b.ordinal)].insert(b.name);
    for (auto const& b : object.bind_lazy) by_dylib[ordinal_name(object, b.ordinal)].insert(b.name);
    for (auto const& [dylib, names] : by_dylib) {
      printf("  imports %-24s %zu\n", dylib.c_str(), names.size());
      if (print_syms)
        for (auto const& n : names) printf("    %s\n", n.c_str());
    }

    ::munmap(file, size);
  }
  return 0;
}
