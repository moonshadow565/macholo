#pragma once

#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

namespace macho {
  struct Entry {
    uint64_t offset;
    uint64_t stack_size;
  };

  struct Section {
    enum Type : uint8_t {
      TYPE_REGULAR = 0x0,
      TYPE_ZEROFILL = 0x1,
      TYPE_CSTRING_LITERALS = 0x2,
      TYPE_4BYTE_LITERALS = 0x3,
      TYPE_8BYTE_LITERALS = 0x4,
      TYPE_LITERAL_POINTERS = 0x5,
      TYPE_NON_LAZY_SYMBOL_POINTERS = 0x6,
      TYPE_LAZY_SYMBOL_POINTERS = 0x7,
      TYPE_SYMBOL_STUBS = 0x8,
      TYPE_MOD_INIT_FUNC_POINTERS = 0x9,
      TYPE_MOD_TERM_FUNC_POINTERS = 0xa,
      TYPE_COALESCED = 0xb,
      TYPE_GB_ZEROFILL = 0xc,
      TYPE_INTERPOSING = 0xd,
      TYPE_16BYTE_LITERALS = 0xe,
      TYPE_DTRACE_DOF = 0xf,
      TYPE_LAZY_DYLIB_SYMBOL_POINTERS = 0x10,
      TYPE_THREAD_LOCAL_REGULAR = 0x11,
      TYPE_THREAD_LOCAL_ZEROFILL = 0x12,
      TYPE_THREAD_LOCAL_VARIABLES = 0x13,
      TYPE_THREAD_LOCAL_VARIABLE_POINTERS = 0x14,
      TYPE_THREAD_LOCAL_INIT_FUNCTION_POINTERS = 0x15,
      TYPE_INIT_FUNC_OFFSETS = 0x16,
    };
    enum [[clang::flag_enum]] Flags {
      FLAG_PURE_INSTRUCTIONS = 0x80000000,
      FLAG_NO_TOC = 0x40000000,
      FLAG_STRIP_STATIC_SYMS = 0x20000000,
      FLAG_NO_DEAD_STRIP = 0x10000000,
      FLAG_LIVE_SUPPORT = 0x08000000,
      FLAG_SELF_MODIFYING_CODE = 0x04000000,
      FLAG_DEBUG = 0x02000000,
      FLAG_SOME_INSTRUCTIONS = 0x00000400,
      FLAG_EXT_RELOC = 0x00000200,
      FLAG_LOC_RELOC = 0x00000100,
    };
    std::string name;
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    Type type;
    Flags flags;
  };

  struct Segment {
    enum [[clang::flag_enum]] Prot {
      VM_PROT_NONE = 0x00,
      VM_PROT_READ = 0x01,
      VM_PROT_WRITE = 0x02,
      VM_PROT_EXECUTE = 0x04,
      VM_PROT_NO_CHANGE = 0x08,
      VM_PROT_COPY = 0x10,
      VM_PROT_DEFAULT = VM_PROT_READ | VM_PROT_WRITE,
      VM_PROT_ALL = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
    };
    enum [[clang::flag_enum]] Flags {
      FLAG_HIGHVM = 0x1,
      FLAG_FVMLIB = 0x2,
      FLAG_NORELOC = 0x4,
      FLAG_PROTECTED_VERSION_1 = 0x8,
      FLAG_READ_ONLY = 0x10,
    };
    std::string name;
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    Prot maxprot;
    Prot initprot;
    Flags flags;
    std::vector<Section> sections;
  };

  struct Rebase {
    enum Type : uint8_t {
      TYPE_POINTER = 1,
      TYPE_TEXT_ABSOLUTE32 = 2,
      TYPE_TEXT_PCREL32 = 3,
    };
    Type type{};
    uint64_t offset{};
  };

  struct Binding {
    enum Type : uint8_t {
      TYPE_POINTER = 1,
      TYPE_TEXT_ABSOLUTE32 = 2,
      TYPE_TEXT_PCREL32 = 3,
    };

    std::string name{};
    Type type{TYPE_POINTER};
    uint8_t flags{};
    int64_t ordinal{};
    uint64_t offset{};
    int64_t value{};
  };

  struct Export {
    enum Type : uint8_t {
      TYPE_REGULAR = 0x00,
      TYPE_LOCAL = 0x01,
      TYPE_ABSOLUTE = 0x02,
    };
    struct ReExport {
      std::string name;
      uint64_t ordinal;
    };
    struct StubAndResolver {
      uint64_t stub_offset;
      uint64_t resolver_offset;
    };
    std::string name;
    Type type;
    std::optional<uint64_t> symbol_offset;
    std::optional<ReExport> reexport;
    std::optional<StubAndResolver> stub_and_resolver;
  };

  // Location of the host-architecture image within a (possibly FAT) file.
  struct Slice {
    size_t offset;
    size_t size;
  };

  // For FAT files returns the slice matching the host architecture, for thin files {0, size}.
  Slice find_slice(void const* file, size_t size);

  struct Error {
    char const* msg{};
    size_t info{};

    constexpr operator bool() const { return !!msg; }
    constexpr bool operator!() const { return !msg; }
  };

  struct Object {
    std::vector<Segment> segments;
    std::vector<Rebase> rebase;
    std::vector<Binding> bind;
    std::vector<Binding> bind_lazy;
    std::vector<Binding> bind_weak;
    std::vector<Export> exports;
    std::optional<Entry> entry;
    size_t text_segment_i;
    size_t data_segment_i;
    size_t tlv_start;
    size_t tlv_size;

    Segment* text_segment() { return text_segment_i ? &segments[text_segment_i] : nullptr; }
    Segment const* text_segment() const { return text_segment_i ? &segments[text_segment_i] : nullptr; }

    Segment* data_segment() { return data_segment_i ? &segments[data_segment_i] : nullptr; }
    Segment const* data_segment() const { return data_segment_i ? &segments[data_segment_i] : nullptr; }

    static Object read(void* image);
  };
}
