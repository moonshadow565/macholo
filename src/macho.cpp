#include "macho.hpp"

#include <algorithm>
#include <array>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "common.hpp"

// #define log_opcode(fmt, ...) printf("%u: " fmt "\n", i++ __VA_OPT__(,) __VA_ARGS__)
#define log_opcode(fmt, ...)

// Using reference: https://github.com/apple/darwin-xnu/blob/main/EXTERNAL_HEADERS/mach-o/loader.h
// Writen to be mostly source compatabile as original but using C++ features for clarity.
namespace macho::raw {
  enum cpu_type_t : std::uint32_t {
    CPU_TYPE_X86 = 7,
    CPU_TYPE_X86_64 = CPU_TYPE_X86 | 0x01000000,
    CPU_TYPE_ARM = 12,
    CPU_TYPE_ARM64 = CPU_TYPE_ARM | 0x01000000,
  };

  enum cpu_subtype_t : std::uint32_t {};

  enum vm_prot_t : std::int32_t {
    VM_PROT_NONE = 0x00,
    VM_PROT_READ = 0x01,
    VM_PROT_WRITE = 0x02,
    VM_PROT_EXECUTE = 0x04,
    VM_PROT_NO_CHANGE = 0x08,
    VM_PROT_COPY = 0x10,
    VM_PROT_DEFAULT = VM_PROT_READ | VM_PROT_WRITE,
    VM_PROT_ALL = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
  };

  enum MAGIC : uint32_t {
    FAT_MAGIC = 0xcafebabe,
    FAT_CIGAM = 0xbebafeca,
    FAT_MAGIC_64 = 0xcafebabf,
    FAT_CIGAM_64 = 0xbfbafeca,
    MH_MAGIC = 0xfeedface,
    MH_CIGAM = 0xcefaedfe,
    MH_MAGIC_64 = 0xfeedfacf,
    MH_CIGAM_64 = 0xcffaedfe,
  };

  enum LC_CMD : uint32_t {
    LC_REQ_DYLD = 0x80000000,
    LC_NONE = 0x0,
    LC_SEGMENT = 0x1,
    LC_SYMTAB = 0x2,
    LC_SYMSEG = 0x3,
    LC_THREAD = 0x4,
    LC_UNIXTHREAD = 0x5,
    LC_LOADFVMLIB = 0x6,
    LC_IDFVMLIB = 0x7,
    LC_IDENT = 0x8,
    LC_FVMFILE = 0x9,
    LC_PREPAGE = 0xa,
    LC_DYSYMTAB = 0xb,
    LC_LOAD_DYLIB = 0xc,
    LC_ID_DYLIB = 0xd,
    LC_LOAD_DYLINKER = 0xe,
    LC_ID_DYLINKER = 0xf,
    LC_PREBOUND_DYLIB = 0x10,
    LC_ROUTINES = 0x11,
    LC_SUB_FRAMEWORK = 0x12,
    LC_SUB_UMBRELLA = 0x13,
    LC_SUB_CLIENT = 0x14,
    LC_SUB_LIBRARY = 0x15,
    LC_TWOLEVEL_HINTS = 0x16,
    LC_PREBIND_CKSUM = 0x17,
    LC_LOAD_WEAK_DYLIB = 0x18 /*| LC_REQ_DYLD*/,
    LC_SEGMENT_64 = 0x19,
    LC_ROUTINES_64 = 0x1a,
    LC_UUID = 0x1b,
    LC_RPATH = 0x1c /*| LC_REQ_DYLD*/,
    LC_CODE_SIGNATURE = 0x1d,
    LC_SEGMENT_SPLIT_INFO = 0x1e,
    LC_REEXPORT_DYLIB = 0x1f /*| LC_REQ_DYLD*/,
    LC_LAZY_LOAD_DYLIB = 0x20,
    LC_ENCRYPTION_INFO = 0x21,
    LC_DYLD_INFO = 0x22,
    LC_LOAD_UPWARD_DYLIB = 0x23 /*| LC_REQ_DYLD*/,
    LC_VERSION_MIN_MACOSX = 0x24,
    LC_VERSION_MIN_IPHONEOS = 0x25,
    LC_FUNCTION_STARTS = 0x26,
    LC_DYLD_ENVIRONMENT = 0x27,
    LC_MAIN = 0x28 /*| LC_REQ_DYLD*/,
    LC_DATA_IN_CODE = 0x29,
    LC_SOURCE_VERSION = 0x2A,
    LC_DYLIB_CODE_SIGN_DRS = 0x2B,
    LC_ENCRYPTION_INFO_64 = 0x2C,
    LC_LINKER_OPTION = 0x2D,
    LC_LINKER_OPTIMIZATION_HINT = 0x2E,
    LC_VERSION_MIN_TVOS = 0x2F,
    LC_VERSION_MIN_WATCHOS = 0x30,
    LC_NOTE = 0x31,
    LC_BUILD_VERSION = 0x32,
    LC_DYLD_EXPORTS_TRIE = 0x33 /*| LC_REQ_DYLD*/,
    LC_DYLD_CHAINED_FIXUPS = 0x34 /*| LC_REQ_DYLD*/,
    LC_FILESET_ENTRY = 0x35 /*| LC_REQ_DYLD*/,
    LC_MAX = 0x36,
  };

  enum MH_FILETYPE : uint32_t {
    MH_OBJECT = 0x1,
    MH_EXECUTE = 0x2,
    MH_FVMLIB = 0x3,
    MH_CORE = 0x4,
    MH_PRELOAD = 0x5,
    MH_DYLIB = 0x6,
    MH_DYLINKER = 0x7,
    MH_BUNDLE = 0x8,
    MH_DYLIB_STUB = 0x9,
    MH_DSYM = 0xa,
    MH_KEXT_BUNDLE = 0xb,
    MH_FILESET = 0xc,
  };

  enum MH_FLAGS : uint32_t {
    MH_NOUNDEFS = 0x1,
    MH_INCRLINK = 0x2,
    MH_DYLDLINK = 0x4,
    MH_BINDATLOAD = 0x8,
    MH_PREBOUND = 0x10,
    MH_SPLIT_SEGS = 0x20,
    MH_LAZY_INIT = 0x40,
    MH_TWOLEVEL = 0x80,
    MH_FORCE_FLAT = 0x100,
    MH_NOMULTIDEFS = 0x200,
    MH_NOFIXPREBINDING = 0x400,
    MH_PREBINDABLE = 0x800,
    MH_ALLMODSBOUND = 0x1000,
    MH_SUBSECTIONS_VIA_SYMBOLS = 0x2000,
    MH_CANONICAL = 0x4000,
    MH_WEAK_DEFINES = 0x8000,
    MH_BINDS_TO_WEAK = 0x10000,
    MH_ALLOW_STACK_EXECUTION = 0x20000,
    MH_ROOT_SAFE = 0x40000,
    MH_SETUID_SAFE = 0x80000,
    MH_NO_REEXPORTED_DYLIBS = 0x100000,
    MH_PIE = 0x200000,
    MH_DEAD_STRIPPABLE_DYLIB = 0x400000,
    MH_HAS_TLV_DESCRIPTORS = 0x800000,
    MH_NO_HEAP_EXECUTION = 0x1000000,
    MH_APP_EXTENSION_SAFE = 0x02000000,
    MH_NLIST_OUTOFSYNC_WITH_DYLDINFO = 0x04000000,
    MH_SIM_SUPPORT = 0x08000000,
    MH_DYLIB_IN_CACHE = 0x80000000,
  };

  enum SECTION_FLAGS : uint32_t {
    S_REGULAR = 0x0,
    S_ZEROFILL = 0x1,
    S_CSTRING_LITERALS = 0x2,
    S_4BYTE_LITERALS = 0x3,
    S_8BYTE_LITERALS = 0x4,
    S_LITERAL_POINTERS = 0x5,
    S_NON_LAZY_SYMBOL_POINTERS = 0x6,
    S_LAZY_SYMBOL_POINTERS = 0x7,
    S_SYMBOL_STUBS = 0x8,
    S_MOD_INIT_FUNC_POINTERS = 0x9,
    S_MOD_TERM_FUNC_POINTERS = 0xa,
    S_COALESCED = 0xb,
    S_GB_ZEROFILL = 0xc,
    S_INTERPOSING = 0xd,
    S_16BYTE_LITERALS = 0xe,
    S_DTRACE_DOF = 0xf,
    S_LAZY_DYLIB_SYMBOL_POINTERS = 0x10,
    S_THREAD_LOCAL_REGULAR = 0x11,
    S_THREAD_LOCAL_ZEROFILL = 0x12,
    S_THREAD_LOCAL_VARIABLES = 0x13,
    S_THREAD_LOCAL_VARIABLE_POINTERS = 0x14,
    S_THREAD_LOCAL_INIT_FUNCTION_POINTERS = 0x15,
    S_INIT_FUNC_OFFSETS = 0x16,
    S_ATTR_PURE_INSTRUCTIONS = 0x80000000,
    S_ATTR_NO_TOC = 0x40000000,
    S_ATTR_STRIP_STATIC_SYMS = 0x20000000,
    S_ATTR_NO_DEAD_STRIP = 0x10000000,
    S_ATTR_LIVE_SUPPORT = 0x08000000,
    S_ATTR_SELF_MODIFYING_CODE = 0x04000000,
    S_ATTR_DEBUG = 0x02000000,
    S_ATTR_SOME_INSTRUCTIONS = 0x00000400,
    S_ATTR_EXT_RELOC = 0x00000200,
    S_ATTR_LOC_RELOC = 0x00000100,
  };

  enum SG_FLAGS : uint32_t {
    SG_HIGHVM = 0x1,
    SG_FVMLIB = 0x2,
    SG_NORELOC = 0x4,
    SG_PROTECTED_VERSION_1 = 0x8,
    SG_READ_ONLY = 0x10,
  };

  enum REBASE_TYPE : uint8_t {
    REBASE_TYPE_POINTER = 1,
    REBASE_TYPE_TEXT_ABSOLUTE32 = 2,
    REBASE_TYPE_TEXT_PCREL32 = 3,
  };

  enum REBASE_OPCODE : uint8_t {
    REBASE_OPCODE_MASK = 0xF0,
    REBASE_IMMEDIATE_MASK = 0x0F,
    REBASE_OPCODE_DONE = 0x00,
    REBASE_OPCODE_SET_TYPE_IMM = 0x10,
    REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB = 0x20,
    REBASE_OPCODE_ADD_ADDR_ULEB = 0x30,
    REBASE_OPCODE_ADD_ADDR_IMM_SCALED = 0x40,
    REBASE_OPCODE_DO_REBASE_IMM_TIMES = 0x50,
    REBASE_OPCODE_DO_REBASE_ULEB_TIMES = 0x60,
    REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB = 0x70,
    REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB = 0x80,
  };

  enum BIND_TYPE : uint8_t {
    BIND_TYPE_POINTER = 1,
    BIND_TYPE_TEXT_ABSOLUTE32 = 2,
    BIND_TYPE_TEXT_PCREL32 = 3,
  };

  enum BIND_ORDINAL : uint32_t {
    BIND_SPECIAL_DYLIB_SELF = 0,
    BIND_SPECIAL_DYLIB_MAIN_EXECUTABLE = (uint32_t)-1,
    BIND_SPECIAL_DYLIB_FLAT_LOOKUP = (uint32_t)-2,
    BIND_SPECIAL_DYLIB_WEAK_LOOKUP = (uint32_t)-3,
  };

  enum BIND_OPCODE : uint8_t {
    BIND_SYMBOL_FLAGS_WEAK_IMPORT = 0x1,
    BIND_SYMBOL_FLAGS_NON_WEAK_DEFINITION = 0x8,

    BIND_OPCODE_MASK = 0xF0,
    BIND_IMMEDIATE_MASK = 0x0F,
    BIND_OPCODE_DONE = 0x00,
    BIND_OPCODE_SET_DYLIB_ORDINAL_IMM = 0x10,
    BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB = 0x20,
    BIND_OPCODE_SET_DYLIB_SPECIAL_IMM = 0x30,
    BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM = 0x40,
    BIND_OPCODE_SET_TYPE_IMM = 0x50,
    BIND_OPCODE_SET_ADDEND_SLEB = 0x60,
    BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB = 0x70,
    BIND_OPCODE_ADD_ADDR_ULEB = 0x80,
    BIND_OPCODE_DO_BIND = 0x90,
    BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB = 0xA0,
    BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED = 0xB0,
    BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB = 0xC0,
    BIND_OPCODE_THREADED = 0xD0,
    BIND_SUBOPCODE_THREADED_SET_BIND_ORDINAL_TABLE_SIZE_ULEB = 0x00,
    BIND_SUBOPCODE_THREADED_APPLY = 0x01,
  };

  enum EXPORT_SYMBOL_FLAG : uint8_t {
    EXPORT_SYMBOL_FLAGS_KIND_MASK = 0x03,
    EXPORT_SYMBOL_FLAGS_KIND_REGULAR = 0x00,
    EXPORT_SYMBOL_FLAGS_KIND_THREAD_LOCAL = 0x01,
    EXPORT_SYMBOL_FLAGS_KIND_ABSOLUTE = 0x02,
    EXPORT_SYMBOL_FLAGS_WEAK_DEFINITION = 0x04,
    EXPORT_SYMBOL_FLAGS_REEXPORT = 0x08,
    EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER = 0x10,
  };

  enum DICE_KIND : uint16_t {
    DICE_KIND_DATA = 0x0001,
    DICE_KIND_JUMP_TABLE8 = 0x0002,
    DICE_KIND_JUMP_TABLE16 = 0x0003,
    DICE_KIND_JUMP_TABLE32 = 0x0004,
    DICE_KIND_ABS_JUMP_TABLE32 = 0x0005,
  };

  enum INDIRECT_SYMBOL : uint32_t {
    INDIRECT_SYMBOL_LOCAL = 0x80000000,
    INDIRECT_SYMBOL_ABS = 0x40000000,
  };

  template <auto S>
  struct off_str {
    uint32_t offset;

    char const* c_str() const { return reinterpret_cast<char const*>(this) - S + offset; }

    std::string_view str_view() const { return c_str(); }
  };

  template <auto S>
  struct small_str {
    char data[S];

    std::string_view str_view() const {
      return std::string_view(data, (size_t)(std::find(data, data + S, '\0') - data));
    }
  };

  template <LC_CMD CMD>
  struct lc_cmd_type;

  template <LC_CMD CMD>
  using lc_cmd_type_t = typename lc_cmd_type<(LC_CMD)(CMD & ~LC_REQ_DYLD)>::type;

  struct load_command {
    LC_CMD cmd;
    uint32_t cmdsize;

    template <LC_CMD CMD>
    lc_cmd_type_t<CMD>* is() {
      return (cmd & ~LC_REQ_DYLD) == (CMD & ~LC_REQ_DYLD) ? static_cast<lc_cmd_type_t<CMD>*>(this) : nullptr;
    }

    template <LC_CMD CMD>
    lc_cmd_type_t<CMD> const* is() const {
      return (cmd & ~LC_REQ_DYLD) == (CMD & ~LC_REQ_DYLD) ? static_cast<lc_cmd_type_t<CMD> const*>(this) : nullptr;
    }

    template <typename...>
    char const* name() const {
      return [cmd = cmd & (~LC_REQ_DYLD)]<size_t... I>(std::index_sequence<I...>) {
        if (char const* name; ((cmd == I ? ((void)(name = lc_cmd_type<(LC_CMD)I>::name), true) : false) || ...))
          return name;
        return "LC_UNK";
      }(std::make_index_sequence<LC_MAX>());
    }
  };

  struct segment_command : load_command {
    small_str<16> segname;
    uint32_t vmaddr;
    uint32_t vmsize;
    uint32_t fileoff;
    uint32_t filesize;
    vm_prot_t maxprot;
    vm_prot_t initprot;
    uint32_t nsects;
    SG_FLAGS flags;
    struct section {
      small_str<16> sectname;
      small_str<16> segname;
      uint32_t addr;
      uint32_t size;
      uint32_t offset;
      uint32_t align;
      uint32_t reloff;
      uint32_t nreloc;
      SECTION_FLAGS flags;
      uint32_t reserved1;
      uint32_t reserved2;
    } sections[0];
  };

  struct symtab_command : load_command {
    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
  };

  struct symseg_command : load_command {
    uint32_t offset;
    uint32_t size;
  };

  struct thread_command : load_command {
    uint32_t kind;
    uint32_t count;
    uint64_t entries[0];
  };

  struct fmvlib_command : load_command {
    off_str<8> name;
    uint32_t minor_version;
    uint32_t header_addr;
  };

  struct ident_command : load_command {
    char data[0];
  };

  struct fmvfile_command : load_command {
    off_str<8> name;
    uint32_t header_addr;
  };

  struct dysymtab_command : load_command {
    uint32_t ilocalsym;
    uint32_t nlocalsym;
    uint32_t iextdefsym;
    uint32_t nextdefsym;
    uint32_t iundefsym;
    uint32_t nundefsym;
    uint32_t tocoff;  // dylib_table_of_contents
    uint32_t ntoc;
    uint32_t modtaboff;  //
    uint32_t nmodtab;
    uint32_t extrefsymoff;
    uint32_t nextrefsyms;
    uint32_t indirectsymoff;
    uint32_t nindirectsyms;
    uint32_t extreloff;
    uint32_t nextrel;
    uint32_t locreloff;
    uint32_t nlocrel;
  };

  struct dylib_command : load_command {
    off_str<8> name;
    uint32_t timestamp;
    uint32_t current_version;
    uint32_t compatibility_version;
  };

  struct dylinker_command : load_command {
    off_str<8> name;
  };

  struct prebound_dylib_command : load_command {
    off_str<8> name;
    uint32_t nmodules;
    off_str<16> linked_modules;
  };

  struct routines_command : load_command {
    uint32_t init_address;
    uint32_t init_module;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t reserved5;
    uint32_t reserved6;
  };

  struct sub_command : load_command {
    off_str<8> sub_name;
  };

  struct twolevel_hint {
    uint32_t isub_image : 8;
    uint32_t itoc : 24;
  };

  struct twolevel_hints_command : load_command {
    uint32_t offset;
    uint32_t nhints;
  };

  struct prebind_cksum_command : load_command {
    uint32_t cksum;
  };

  struct segment_command_64 : load_command {
    small_str<16> segname;
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    vm_prot_t maxprot;
    vm_prot_t initprot;
    uint32_t nsects;
    SG_FLAGS flags;
    struct section {
      small_str<16> sectname;
      small_str<16> segname;
      uint64_t addr;
      uint64_t size;
      uint32_t offset;
      uint32_t align;
      uint32_t reloff;
      uint32_t nreloc;
      SECTION_FLAGS flags;
      uint32_t reserved1;
      uint32_t reserved2;
      uint32_t reserved3;
    } sections[0];

    section const* begin() const { return &sections[0]; }
    section const* end() const { return &sections[nsects]; }
  };

  struct routines_command_64 : load_command {
    uint64_t init_address;
    uint64_t init_module;
    uint64_t reserved1;
    uint64_t reserved2;
    uint64_t reserved3;
    uint64_t reserved4;
    uint64_t reserved5;
    uint64_t reserved6;
  };

  struct uuid_command : load_command {
    std::array<uint8_t, 16> uuid;
  };

  struct rpath_command : load_command {
    off_str<8> path;
  };

  struct linkedit_data_command : load_command {
    uint32_t dataoff;
    uint32_t datasize;
  };

  struct encryption_info_command : load_command {
    uint32_t cryptoff;
    uint32_t cryptsize;
    uint32_t cryptid;
  };

  struct dyld_info_command : load_command {
    uint32_t rebase_off;
    uint32_t rebase_size;
    uint32_t bind_off;
    uint32_t bind_size;
    uint32_t weak_bind_off;
    uint32_t weak_bind_size;
    uint32_t lazy_bind_off;
    uint32_t lazy_bind_size;
    uint32_t export_off;
    uint32_t export_size;
  };

  struct version_min_command : load_command {
    uint32_t version;
    uint32_t sdk;
  };

  struct main_command : load_command {
    uint64_t entryoff;
    uint64_t stacksize;
  };

  struct source_version_command : load_command {
    uint64_t version;
  };

  struct encryption_info_command_64 : load_command {
    uint32_t cryptoff;
    uint32_t cryptsize;
    uint32_t cryptid;
    uint32_t align64;
  };

  struct linker_option_command : load_command {
    uint32_t count;
    char data[0];
  };

  struct note_command : load_command {
    small_str<16> data_owner;
    uint64_t offset;
    uint64_t size;
  };

  struct build_version_command : load_command {
    uint32_t platform;
    uint32_t minos;
    uint32_t sdk;
    uint32_t ntools;
  };

  struct fileset_entry_command : load_command {
    uint64_t vmaddr;
    uint64_t fileoff;
    off_str<24> entry_id;
    uint32_t reserved;
  };

  struct data_in_code_entry {
    uint32_t offset;
    uint16_t length;
    DICE_KIND kind;
  };

#define impl_command($cmd, $base)                     \
  template <>                                         \
  struct lc_cmd_type<(LC_CMD)($cmd & ~LC_REQ_DYLD)> { \
    using type = $base;                               \
    static constexpr char name[] = #$cmd;             \
  }
  impl_command(LC_NONE, load_command);
  impl_command(LC_SEGMENT, segment_command);
  impl_command(LC_SYMTAB, symtab_command);
  impl_command(LC_SYMSEG, symseg_command);
  impl_command(LC_THREAD, thread_command);
  impl_command(LC_UNIXTHREAD, thread_command);
  impl_command(LC_LOADFVMLIB, fmvlib_command);
  impl_command(LC_IDFVMLIB, fmvlib_command);
  impl_command(LC_IDENT, ident_command);
  impl_command(LC_FVMFILE, fmvfile_command);
  impl_command(LC_PREPAGE, load_command);
  impl_command(LC_DYSYMTAB, dysymtab_command);
  impl_command(LC_LOAD_DYLIB, dylib_command);
  impl_command(LC_ID_DYLIB, dylib_command);
  impl_command(LC_LOAD_DYLINKER, dylinker_command);
  impl_command(LC_ID_DYLINKER, dylinker_command);
  impl_command(LC_PREBOUND_DYLIB, prebound_dylib_command);
  impl_command(LC_ROUTINES, routines_command);
  impl_command(LC_SUB_FRAMEWORK, sub_command);
  impl_command(LC_SUB_UMBRELLA, sub_command);
  impl_command(LC_SUB_CLIENT, sub_command);
  impl_command(LC_SUB_LIBRARY, sub_command);
  impl_command(LC_TWOLEVEL_HINTS, twolevel_hints_command);
  impl_command(LC_PREBIND_CKSUM, prebind_cksum_command);
  impl_command(LC_LOAD_WEAK_DYLIB, dylib_command);
  impl_command(LC_SEGMENT_64, segment_command_64);
  impl_command(LC_ROUTINES_64, routines_command_64);
  impl_command(LC_UUID, uuid_command);
  impl_command(LC_RPATH, rpath_command);
  impl_command(LC_CODE_SIGNATURE, linkedit_data_command);
  impl_command(LC_SEGMENT_SPLIT_INFO, linkedit_data_command);
  impl_command(LC_REEXPORT_DYLIB, dylib_command);
  impl_command(LC_LAZY_LOAD_DYLIB, load_command);
  impl_command(LC_ENCRYPTION_INFO, encryption_info_command);
  impl_command(LC_DYLD_INFO, dyld_info_command);
  impl_command(LC_LOAD_UPWARD_DYLIB, dylib_command);
  impl_command(LC_VERSION_MIN_MACOSX, version_min_command);
  impl_command(LC_VERSION_MIN_IPHONEOS, version_min_command);
  impl_command(LC_FUNCTION_STARTS, linkedit_data_command);
  impl_command(LC_DYLD_ENVIRONMENT, dylinker_command);
  impl_command(LC_MAIN, main_command);
  impl_command(LC_DATA_IN_CODE, linkedit_data_command);
  impl_command(LC_SOURCE_VERSION, source_version_command);
  impl_command(LC_DYLIB_CODE_SIGN_DRS, linkedit_data_command);
  impl_command(LC_ENCRYPTION_INFO_64, encryption_info_command_64);
  impl_command(LC_LINKER_OPTION, linker_option_command);
  impl_command(LC_LINKER_OPTIMIZATION_HINT, linkedit_data_command);
  impl_command(LC_VERSION_MIN_TVOS, version_min_command);
  impl_command(LC_VERSION_MIN_WATCHOS, version_min_command);
  impl_command(LC_NOTE, note_command);
  impl_command(LC_BUILD_VERSION, build_version_command);
  impl_command(LC_DYLD_EXPORTS_TRIE, linkedit_data_command);
  impl_command(LC_DYLD_CHAINED_FIXUPS, linkedit_data_command);
  impl_command(LC_FILESET_ENTRY, fileset_entry_command);
#undef impl_command

  struct iter_command {
    constexpr iter_command() = default;

    constexpr iter_command(load_command const* cmd) : cmd_(cmd) {}

    load_command const* operator->() const { return cmd_; }

    load_command const& operator*() const { return *cmd_; }

    iter_command& operator++() {
      cmd_ = (load_command const*)((char const*)cmd_ + cmd_->cmdsize);
      return *this;
    }

    iter_command operator++(int) {
      iter_command old = *this;
      (*this)++;
      return old;
    }

    constexpr bool operator==(iter_command other) const { return cmd_ == other.cmd_; }

    constexpr bool operator!=(iter_command other) const { return cmd_ != other.cmd_; }

  private:
    load_command const* cmd_{};
  };

  struct mach_header_64 {
    uint32_t magic;
    cpu_type_t cputype;
    cpu_subtype_t cpusubtype;
    MH_FILETYPE filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    MH_FLAGS flags;
    uint32_t reserved;
    load_command cmds[0];

    iter_command begin() const { return &cmds[0]; }
    iter_command end() const { return (load_command const*)((char const*)this + sizeofcmds); }

    template <LC_CMD CMD>
    lc_cmd_type_t<CMD> const* get() const {
      for (auto const& command : *this)
        if (const auto c = command.is<CMD>()) return c;
      return nullptr;
    }

    template <LC_CMD CMD>
    lc_cmd_type_t<CMD> const* get(size_t n) const {
      for (auto const& command : *this)
        if (const auto c = command.is<CMD>())
          if (n-- == 0) return c;
      return nullptr;
    }
  };

  struct fat_header {
    uint32_t magic;
    uint32_t nfat_arch;

    struct arch {
      cpu_type_t cputype;
      cpu_subtype_t cpusubtype;
      uint32_t offset;
      uint32_t size;
      uint32_t align;
    } archs[0];
  };

  struct binary_reader;
}

struct macho::raw::binary_reader {
  constexpr binary_reader() = default;

  constexpr binary_reader(const char* data, size_t size) : data_(data), size_(size) {}

  const char* data() const { return data_; }

  size_t size() const { return size_; }

  bool empty() const { return !size_; }

  binary_reader copy(std::size_t offset = 0, std::size_t count = (std::size_t)-1) const {
    assert_that(size_ >= offset);
    auto const remain_data = size_ - offset;
    if (count == (std::size_t)-1) count = remain_data;
    assert_that(remain_data >= count);
    return binary_reader{data_ + offset, count};
  }

  template <typename T>
  T read_raw() {
    assert_that(size_ >= sizeof(T));
    unsigned char raw[sizeof(T)];
    memcpy(raw, data_, sizeof(T));
    T result;
    memcpy(&result, raw, sizeof(T));
    data_ = data_ + sizeof(T);
    size_ -= sizeof(T);
    return result;
  }

  std::uint64_t read_uleb64() {
    char const* data = data_;
    std::uint64_t result = 0;
    for (int shift = 0; true; shift += 7) {
      auto const c = read_raw<std::uint8_t>();
      result |= ((std::uint64_t)(c & 0x7f) << shift);
      if (!(c & 0x80)) {
        break;
      }
    }
    return result;
  }

  std::int64_t read_sleb64() {
    char const* data = data_;
    std::int64_t result = 0;
    for (int shift = 0; true; shift += 7) {
      auto const c = read_raw<std::uint8_t>();
      result |= ((std::uint64_t)(c & 0x7f) << shift);
      if (!(c & 0x80)) {
        result <<= 64 - (shift + 7);
        result >>= 64 - (shift + 7);
        break;
      }
    }
    return result;
  }

  void skip(std::size_t count) {
    data_ += count;
    size_ -= count;
  }

  char const* read_zstring() {
    char const* result = data_;
    for (;;) {
      assert_that(size_);
      char const c = *data_;
      ++data_;
      --size_;
      if (!c) break;
    }
    return result;
  }

  std::vector<Rebase> read_rebase(std::vector<macho::Segment> const& segments) {
    std::vector<Rebase> results;
    struct State {
      uint8_t type{};
      uint8_t segment{};
      uint64_t offset{};
    } state{};
    auto push = [&](size_t skip_extra) {
      results.push_back(Rebase{
          .type = (Rebase::Type)state.type,
          .offset = segments[state.segment].vmaddr + state.offset,
      });
      state.offset += sizeof(uintptr_t);
      state.offset += skip_extra;
    };

    [[maybe_unused]] unsigned i = 0;
    for (auto& stream = *this; !stream.empty();) {
      auto const raw_opcode = stream.read_raw<std::uint8_t>();
      auto const opcode = raw_opcode & REBASE_OPCODE_MASK;
      auto const immediate = raw_opcode & REBASE_IMMEDIATE_MASK;
      switch (opcode) {
        case REBASE_OPCODE_DONE: {
          log_opcode("REBASE_OPCODE_DONE");
        } break;
        case REBASE_OPCODE_SET_TYPE_IMM: {
          log_opcode("REBASE_OPCODE_SET_TYPE_IMM(%u)", immediate);
          state.type = immediate;
        } break;
        case REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB: {
          const auto offset = stream.read_uleb64();
          log_opcode("REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB(%u)", immediate);
          log_opcode("ULEB(%lu)", offset);
          state.segment = immediate;
          state.offset = offset;
        } break;
        case REBASE_OPCODE_ADD_ADDR_ULEB: {
          const auto add = (int64_t)stream.read_uleb64();
          log_opcode("REBASE_OPCODE_ADD_ADDR_ULEB");
          log_opcode("ULEB(%ld)", add);
          state.offset += add;
        } break;
        case REBASE_OPCODE_ADD_ADDR_IMM_SCALED: {
          log_opcode("REBASE_OPCODE_ADD_ADDR_IMM_SCALED(%u)", immediate);
          state.offset += immediate * sizeof(uintptr_t);
        } break;
        case REBASE_OPCODE_DO_REBASE_IMM_TIMES: {
          log_opcode("REBASE_OPCODE_DO_REBASE_IMM_TIMES(%u)", immediate);
          for (auto i = immediate; i > 0; --i) push(0);
        } break;
        case REBASE_OPCODE_DO_REBASE_ULEB_TIMES: {
          const auto count = (int64_t)read_uleb64();
          log_opcode("REBASE_OPCODE_DO_REBASE_ULEB_TIMES");
          log_opcode("ULEB(%ld)", count);
          for (auto i = count; i > 0; --i) push(0);
        } break;
        case REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB: {
          const auto add = (int64_t)stream.read_uleb64();
          log_opcode("REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB");
          log_opcode("ULEB(%ld)", add);
          push(add);
        } break;
        case REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB: {
          const auto count = (int64_t)read_uleb64();
          const auto skip = (int64_t)read_uleb64();
          log_opcode("REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB");
          log_opcode("ULEB(%ld)", count);
          log_opcode("ULEB(%lu)", skip);
          for (auto i = count; i > 0; --i) push(skip);
        } break;
        default:
          panic("Unknown rebase opcode: %x", opcode);
      }
    }
    return results;
  }

  std::vector<Binding> read_bindings(std::vector<macho::Segment> const& segments) {
    std::vector<Binding> results;
    struct State {
      std::string name{};
      uint8_t type = BIND_TYPE_POINTER;
      uint8_t flags{};
      uint8_t segment{};
      int64_t ordinal{};
      uint64_t offset{};
      int64_t value{};
    } state{};
    auto push = [&](size_t skip_extra) {
      results.push_back({
          .name = state.name,
          .type = (Binding::Type)(state.type),
          .flags = state.flags,
          .ordinal = state.ordinal,
          .offset = segments[state.segment].vmaddr + state.offset,
      });
      state.offset += sizeof(uintptr_t);
      state.offset += skip_extra;
    };

    [[maybe_unused]] unsigned i = 0;
    for (auto& stream = *this; !stream.empty();) {
      auto const raw_opcode = stream.read_raw<std::uint8_t>();
      auto const opcode = raw_opcode & BIND_OPCODE_MASK;
      auto const immediate = raw_opcode & BIND_IMMEDIATE_MASK;
      switch (opcode) {
        case BIND_OPCODE_DONE: {
          log_opcode("BIND_OPCODE_DONE");
          state = {};
        } break;
        case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM: {
          log_opcode("BIND_OPCODE_SET_DYLIB_ORDINAL_IMM(%u)", immediate);
          state.ordinal = immediate;
        } break;
        case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB: {
          const auto value = stream.read_uleb64();
          log_opcode("BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB");
          log_opcode("ULEB128(%lu)", value);
          state.ordinal = value;
        } break;
        case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM: {
          log_opcode("BIND_OPCODE_SET_DYLIB_SPECIAL_IMM(%u)", immediate);
          state.ordinal = immediate ? (immediate - std::int64_t{16}) : 0;
        } break;
        case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM: {
          const auto str = stream.read_zstring();
          log_opcode("BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM(%u)", immediate);
          log_opcode("String(%s)", str);
          state.name = str;
          state.flags = immediate;
        } break;
        case BIND_OPCODE_SET_TYPE_IMM: {
          state.type = (Binding::Type)immediate;
          log_opcode("BIND_OPCODE_SET_TYPE_IMM(%u)", immediate);
        } break;
        case BIND_OPCODE_SET_ADDEND_SLEB: {
          const auto value = stream.read_sleb64();
          log_opcode("BIND_OPCODE_SET_ADDEND_SLEB");
          log_opcode("SLEB128(%ld)", value);
          state.value = value;
        } break;
        case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB: {
          const auto offset = stream.read_uleb64();
          log_opcode("BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB(%u)", immediate);
          log_opcode("ULEB128(%lu)", offset);
          state.segment = immediate;
          state.offset = offset;
        } break;
        case BIND_OPCODE_ADD_ADDR_ULEB: {
          const auto add = (std::int64_t)stream.read_uleb64();
          log_opcode("BIND_OPCODE_ADD_ADDR_ULEB");
          log_opcode("ULEB128(%ld)", add);
          state.offset += add;
        } break;
        case BIND_OPCODE_DO_BIND: {
          log_opcode("BIND_OPCODE_DO_BIND");
          push(0);
        } break;
        case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB: {
          const auto add = stream.read_uleb64();
          log_opcode("BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB");
          log_opcode("ULEB128(%lu)", add);
          push(add);
        } break;
        case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED: {
          log_opcode("BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED(%u)", immediate);
          push((size_t)immediate * sizeof(uintptr_t));
        } break;
        case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB: {
          auto const count = (int64_t)stream.read_uleb64();
          auto const skip = stream.read_uleb64();
          log_opcode("BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB");
          log_opcode("ULEB128(%ld)", count);
          log_opcode("ULEB128(%lu)", skip);
          for (auto i = count; i > 0; --i) push(skip);
        } break;
        default:
          panic("Unknown binding opcode: %x", opcode);
      }
    }
    return results;
  }

  std::vector<Export> read_exports(std::vector<macho::Segment> const& segments) {
    std::vector<Export> results;
    auto& org_stream = *this;
    auto stack = std::vector<std::pair<std::string, std::size_t>>{{"", 0ull}};
    while (!stack.empty()) {
      auto const [sym_name, offset] = stack.back();
      stack.pop_back();
      auto stream = org_stream.copy(offset);
      if (auto const info_len = stream.read_uleb64()) {
        auto const flags = stream.read_uleb64();
        auto const type = (Export::Type)(flags & EXPORT_SYMBOL_FLAGS_KIND_MASK);
        if (flags & EXPORT_SYMBOL_FLAGS_REEXPORT) {
          auto const ordinal = stream.read_uleb64();
          auto const name = stream.read_zstring();
          results.push_back(Export{
              .name = sym_name,
              .type = type,
              .reexport =
                  Export::ReExport{
                      .name = name,
                      .ordinal = ordinal,
                  },
          });
        } else if (flags & EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER) {
          auto const stub_offset = stream.read_uleb64();
          auto const resolver_offset = stream.read_uleb64();
          results.push_back(Export{
              .name = sym_name,
              .type = type,
              .stub_and_resolver =
                  Export::StubAndResolver{
                      .stub_offset = stub_offset,
                      .resolver_offset = resolver_offset,
                  },
          });
        } else {
          auto const symbol_offset = stream.read_uleb64();
          results.push_back(Export{
              .name = sym_name,
              .type = type,
              .symbol_offset = symbol_offset,
          });
        }
      }
      for (auto child_count = (std::uint32_t)stream.read_raw<std::uint8_t>(); child_count; --child_count) {
        auto const child_name = sym_name + stream.read_zstring();
        auto const child_offset = stream.read_uleb64();
        stack.emplace_back(child_name, child_offset);
      }
    }
    return results;
  }

private:
  const char* data_{};
  size_t size_{};
};

macho::Object macho::Object::read(void* image) {
  using namespace macho::raw;

  auto object = macho::Object{};

  auto const mach = (mach_header_64*)image;
  assert_that(mach->magic == MH_MAGIC_64);

  // Debug print cmds.
  // for (const auto& cmd : *mach)
  //   printf("cmd: %s\n", cmd.name());

  // Read segments.
  for (const auto& cmd : *mach) {
    if (auto const cmd_seg = cmd.is<LC_SEGMENT_64>()) {
      Segment seg = {
          .name = std::string(cmd_seg->segname.str_view()),
          .vmaddr = cmd_seg->vmaddr,
          .vmsize = cmd_seg->vmsize,
          .fileoff = cmd_seg->fileoff,
          .filesize = cmd_seg->filesize,
          .maxprot = (Segment::Prot)cmd_seg->maxprot,
          .initprot = (Segment::Prot)cmd_seg->initprot,
          .flags = (Segment::Flags)cmd_seg->flags,
      };
      for (auto const& cmd_sec : *cmd_seg) {
        Section sec{
            .name = std::string(cmd_sec.sectname.str_view()),
            .addr = cmd_sec.addr,
            .size = cmd_sec.size,
            .offset = cmd_sec.offset,
            .align = cmd_sec.align,
            .type = (Section::Type)(cmd_sec.flags & 0xFF),
            .flags = (Section::Flags)(cmd_sec.flags & 0xFFFFFF00),
        };
        seg.sections.push_back(sec);
        if (sec.type == Section::TYPE_THREAD_LOCAL_REGULAR || sec.type == Section::TYPE_THREAD_LOCAL_ZEROFILL) {
          // if there is no start currently or addr is less than tlv_start, add it, else use old
          object.tlv_start = !object.tlv_start || sec.addr < object.tlv_start ? sec.addr : object.tlv_start;
          // recalculate size, align to 0x10
          constexpr auto align = 0x10;
          auto const size = ((((sec.addr + sec.size) - object.tlv_start) + align - 1) / align) * align;
          // if there is no new size or new size is bigger, add it, else use old
          object.tlv_size = !object.tlv_size || size > object.tlv_size ? size : object.tlv_size;
        }
      }
      if (seg.name == "__TEXT")
        object.text_segment_i = object.segments.size();
      if (seg.name == "__DATA")
        object.data_segment_i = object.segments.size();
      object.segments.push_back(seg);
    }
  }

  // Read dependent dylibs and rpaths. All dependent-dylib command kinds share one ordinal space.
  for (const auto& cmd : *mach) {
    if (auto const c = cmd.is<LC_LOAD_DYLIB>()) {
      object.dylibs.push_back({std::string(c->name.str_view()), false});
    } else if (auto const c = cmd.is<LC_LOAD_WEAK_DYLIB>()) {
      object.dylibs.push_back({std::string(c->name.str_view()), true});
    } else if (auto const c = cmd.is<LC_REEXPORT_DYLIB>()) {
      object.dylibs.push_back({std::string(c->name.str_view()), false});
    } else if (auto const c = cmd.is<LC_LOAD_UPWARD_DYLIB>()) {
      object.dylibs.push_back({std::string(c->name.str_view()), false});
    } else if (auto const c = cmd.is<LC_RPATH>()) {
      object.rpaths.push_back(std::string(c->path.str_view()));
    }
  }

  // Read rebase.
  if (auto const info = mach->get<LC_DYLD_INFO>()) {
    object.rebase = binary_reader((char*)mach + info->rebase_off, info->rebase_size).read_rebase(object.segments);
    object.bind = binary_reader((char*)mach + info->bind_off, info->bind_size).read_bindings(object.segments);
    object.bind_lazy = binary_reader((char*)mach + info->lazy_bind_off, info->lazy_bind_size).read_bindings(object.segments);
    object.bind_weak = binary_reader((char*)mach + info->weak_bind_off, info->weak_bind_size).read_bindings(object.segments);
    object.exports = binary_reader((char*)mach + info->export_off, info->export_size).read_exports(object.segments);
  }

  if (auto const main = mach->get<LC_MAIN>()) {
    object.entry = Entry{
        .offset = main->entryoff,
        .stack_size = main->stacksize,
    };
  }

  return object;
}

macho::Slice macho::find_slice(void const* file, size_t size) {
  using namespace macho::raw;

#if defined(__x86_64__)
  constexpr auto host_cputype = CPU_TYPE_X86_64;
#elif defined(__aarch64__)
  constexpr auto host_cputype = CPU_TYPE_ARM64;
#else
#error "unsupported host architecture"
#endif

  assert_that(size >= sizeof(uint32_t));
  uint32_t magic;
  memcpy(&magic, file, sizeof(magic));

  if (magic != FAT_MAGIC && magic != FAT_CIGAM) {
    assert_that(magic != FAT_MAGIC_64 && magic != FAT_CIGAM_64);  // fat_arch_64 unsupported
    return {0, size};
  }

  // FAT headers are big-endian on disk.
  auto const swap = magic == FAT_CIGAM ? +[](uint32_t v) { return __builtin_bswap32(v); }  //
                                       : +[](uint32_t v) { return v; };
  auto const fat = (fat_header const*)file;
  auto const nfat_arch = swap(fat->nfat_arch);
  assert_that(size >= sizeof(fat_header) + nfat_arch * sizeof(fat_header::arch));
  for (uint32_t i = 0; i != nfat_arch; ++i) {
    auto const& arch = fat->archs[i];
    if (swap(arch.cputype) != host_cputype) continue;
    auto const offset = swap(arch.offset);
    auto const arch_size = swap(arch.size);
    assert_that(offset <= size && arch_size <= size - offset);
    return {offset, arch_size};
  }
  panic("no Mach-O slice for host architecture");
}
