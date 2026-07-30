#include <sys/mman.h>

#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "common.hpp"
#include "macho.hpp"

// ---------------------------------------------------------------------------
// ObjC surface reporting.
//
// The file is mapped read-only and *not* laid out at its vmaddrs, so every
// pointer stored inside it (an unslid vmaddr, since the image is not rebased)
// has to be translated back to a file offset before it can be dereferenced.
// Struct layouts below mirror objc::raw in src/objc.cpp, but with every pointer
// spelled as a uint64_t vmaddr; they are kept local on purpose so that this
// tool does not drag in objc.hpp / the GNUstep runtime headers.
// ---------------------------------------------------------------------------
namespace objcdump {
  struct method_list_t {
    uint32_t entsize_and_flags;
    uint32_t count;
  };

  struct class_ro_t {
    uint32_t flags;
    uint32_t ivar_base_start;
    uint32_t ivar_base_size;
    uint32_t reserved;
    uint64_t ivar_lyt;
    uint64_t name;
    uint64_t base_methods;
    uint64_t base_protocols;
    uint64_t ivars;
    uint64_t weak_ivar_lyt;
    uint64_t base_props;
  };

  struct class_t {
    uint64_t isa;
    uint64_t superclass;
    uint64_t cache;
    uint64_t vtable;
    uint64_t bits;

    uint64_t data() const { return bits & ~uint64_t{0b11}; }
  };

  struct category_t {
    uint64_t name;
    uint64_t cls;
    uint64_t methods;
    uint64_t static_methods;
    uint64_t protocols;
    uint64_t properties;
  };

  // A window into the mapped file, plus vmaddr -> file-offset translation.
  struct Ctx {
    char const* file{};
    size_t file_size{};
    macho::Slice slice{};
    macho::Object const* object{};
    // vmaddr -> symbol, for pointer fields that are patched by dyld at load
    // time and therefore read as 0 (or stale garbage) in the on-disk image.
    std::map<uint64_t, std::string> binds{};
    mutable size_t bad_ptrs{};

    // Bytes readable at `addr`, 0 if `addr` is not backed by file content.
    size_t readable(uint64_t addr, char const** out) const {
      if (!addr) return 0;
      for (auto const& seg : object->segments) {
        if (!seg.filesize || addr < seg.vmaddr || addr - seg.vmaddr >= seg.filesize) continue;
        uint64_t const in_seg = addr - seg.vmaddr;
        uint64_t const off = (uint64_t)slice.offset + seg.fileoff + in_seg;
        // Clamp to the enclosing slice as well as to the mapping itself.
        uint64_t const slice_end = (uint64_t)slice.offset + slice.size;
        if (off >= slice_end || slice_end > file_size) return 0;
        uint64_t avail = seg.filesize - in_seg;
        if (avail > slice_end - off) avail = slice_end - off;
        *out = file + off;
        return (size_t)avail;
      }
      return 0;
    }

    // Typed read; returns nullptr (and bumps bad_ptrs) for anything unmapped.
    template <typename T>
    T const* get(uint64_t addr) const {
      char const* p{};
      if (readable(addr, &p) < sizeof(T)) {
        if (addr) ++bad_ptrs;
        return nullptr;
      }
      return (T const*)p;
    }

    // NUL-terminated string that must terminate inside the mapped region.
    char const* str(uint64_t addr) const {
      char const* p{};
      size_t const avail = readable(addr, &p);
      if (!avail) {
        if (addr) ++bad_ptrs;
        return nullptr;
      }
      if (!::memchr(p, '\0', avail)) {
        ++bad_ptrs;
        return nullptr;
      }
      return p;
    }

    macho::Section const* find_section(std::string_view name) const {
      for (auto const& seg : object->segments)
        for (auto const& sec : seg.sections)
          if (sec.name == name) return &sec;
      return nullptr;
    }

    // Contents of a section as an array of vmaddr-sized entries.
    std::vector<uint64_t> pointer_section(std::string_view name) const {
      std::vector<uint64_t> out;
      auto const* sec = find_section(name);
      if (!sec || !sec->size) return out;
      char const* p{};
      size_t avail = readable(sec->addr, &p);
      if (avail > sec->size) avail = sec->size;
      for (size_t i = 0; i + sizeof(uint64_t) <= avail; i += sizeof(uint64_t)) {
        uint64_t v;
        ::memcpy(&v, p + i, sizeof(v));
        out.push_back(v);
      }
      return out;
    }

    // Name of the class at `addr`: resolved locally, or via the bind table when
    // the slot is an unbound import.
    std::string class_name(uint64_t addr, uint64_t slot_addr) const {
      if (addr) {
        if (auto const* cls = get<class_t>(addr))
          if (auto const* ro = get<class_ro_t>(cls->data()))
            if (auto const* name = str(ro->name)) return name;
      }
      if (slot_addr) {
        if (auto const it = binds.find(slot_addr); it != binds.end()) {
          auto const& sym = it->second;
          std::string_view const prefix = "_OBJC_CLASS_$_";
          if (sym.starts_with(prefix)) return sym.substr(prefix.size()) + " (imported)";
          return sym + " (imported)";
        }
      }
      return addr ? "(unresolved)" : "(imported)";
    }

    uint32_t method_count(uint64_t addr) const {
      auto const* list = get<method_list_t>(addr);
      return list ? list->count : 0;
    }
  };

  static constexpr std::string_view CLASS_PREFIX = "_OBJC_CLASS_$_";

  static void report(macho::Object const& object, void const* file, size_t file_size, macho::Slice slice, bool verbose) {
    Ctx ctx{.file = (char const*)file, .file_size = file_size, .slice = slice, .object = &object};
    for (auto const* list : {&object.bind, &object.bind_lazy, &object.bind_weak})
      for (auto const& b : *list) ctx.binds.emplace(b.offset, b.name);

    // 1. Defined classes.
    struct Defined {
      std::string name;
      std::string super;
      uint32_t methods;
    };
    std::vector<Defined> defined;
    for (auto const addr : ctx.pointer_section("__objc_classlist")) {
      if (!addr) continue;  // null slots do occur in these lists
      auto const* cls = ctx.get<class_t>(addr);
      if (!cls) continue;
      auto const* ro = ctx.get<class_ro_t>(cls->data());
      if (!ro) continue;
      auto const* name = ctx.str(ro->name);
      if (!name) continue;
      // The superclass slot lives at offsetof(class_t, superclass) in the image.
      defined.push_back({name, ctx.class_name(cls->superclass, addr + offsetof(class_t, superclass)),
                         ctx.method_count(ro->base_methods)});
    }

    // 2. Imported classes (from the bind opcodes, already parsed).
    std::set<std::string> imported;
    for (auto const* list : {&object.bind, &object.bind_lazy})
      for (auto const& b : *list)
        if (std::string_view(b.name).starts_with(CLASS_PREFIX)) imported.insert(b.name.substr(CLASS_PREFIX.size()));

    // 3. Referenced selectors: __objc_selrefs is an array of char* into __objc_methname.
    std::set<std::string> selectors;
    for (auto const addr : ctx.pointer_section("__objc_selrefs")) {
      if (!addr) continue;
      if (auto const* name = ctx.str(addr)) selectors.insert(name);
    }

    // 4. Categories.
    struct Category {
      std::string name;
      std::string cls;
      uint32_t methods;
      uint32_t static_methods;
    };
    std::vector<Category> categories;
    for (auto const addr : ctx.pointer_section("__objc_catlist")) {
      if (!addr) continue;
      auto const* cat = ctx.get<category_t>(addr);
      if (!cat) continue;
      auto const* name = ctx.str(cat->name);
      if (!name) continue;
      categories.push_back({name, ctx.class_name(cat->cls, addr + offsetof(category_t, cls)),
                            ctx.method_count(cat->methods), ctx.method_count(cat->static_methods)});
    }

    if (defined.empty() && imported.empty() && selectors.empty() && categories.empty() && !ctx.bad_ptrs) return;
    printf("  objc: classes=%zu imported=%zu selectors=%zu categories=%zu\n", defined.size(), imported.size(),
           selectors.size(), categories.size());
    if (ctx.bad_ptrs) printf("  objc: %zu unmapped/corrupt pointer(s) skipped\n", ctx.bad_ptrs);
    if (!verbose) return;

    for (auto const& d : defined)
      printf("    class    %-32s : %-32s methods=%u\n", d.name.c_str(), d.super.c_str(), d.methods);
    for (auto const& c : categories)
      printf("    category %s(%s) methods=%u static=%u\n", c.cls.c_str(), c.name.c_str(), c.methods, c.static_methods);
    for (auto const& i : imported) printf("    imported class %s\n", i.c_str());
    for (auto const& s : selectors) printf("    selector %s\n", s.c_str());
  }
}

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
  bool print_objc = false;
  int argi = 1;
  for (; argi < argc; ++argi) {
    auto const arg = std::string_view(argv[argi]);
    if (arg == "--syms")
      print_syms = true;
    else if (arg == "--objc")
      print_objc = true;
    else
      break;
  }
  if (argi == argc) {
    fprintf(stderr, "usage: %s [--syms] [--objc] <macho-file>...\n", argv[0]);
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

    objcdump::report(object, file, size, sl, print_objc);

    ::munmap(file, size);
  }
  return 0;
}
