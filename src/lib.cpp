#include "lib.hpp"

#include <dlfcn.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "loader.hpp"

// Stubs that interact with loader in some capacity cant be placed in external library.
namespace lib::stub {
  static unsigned long get_end() {
    // Used by statistics, doesn't matter but might as well implement it properly.
    auto const data = loader::Image::main->object.text_segment();
    assert_that(data != nullptr);
    return loader::Image::main->slide + data->vmaddr + data->vmsize;
  }

  static unsigned long get_etext() {
    // Used by statistics, doesn't matter but might as well implement it properly.
    auto const text = loader::Image::main->object.text_segment();
    assert_that(text != nullptr);
    return loader::Image::main->slide + text->vmaddr + text->vmsize;
  }

  static unsigned long get_edata() {
    // Used by statistics, doesn't matter but might as well implement it properly.
    auto const data = loader::Image::main->object.text_segment();
    assert_that(data != nullptr);
    return loader::Image::main->slide + data->vmaddr + data->vmsize;
  }

  static int NSGetExecutablePath(char *buf, uint32_t *bufsize) {
    if (!bufsize) {
      return -1;
    }
    const auto required = (uint32_t)(loader::Image::main->path.size() + 1);
    if (required > *bufsize) {
      *bufsize = required;
      return -1;
    }
    memcpy(buf, loader::Image::main->path.c_str(), required);
    *bufsize = required;
    return 0;
  }

  static void dyld_register_func_for_add_image(void (*func)(const struct mach_header *mh, intptr_t vmaddr_slide)) {
    // NOTE: crashpad stuff, useless..
  }

  static void dyld_register_func_for_remove_image(void (*func)(const struct mach_header *mh, intptr_t vmaddr_slide)) {
    // NOTE: crashpad stuff, useles...
  }

  // [image_begin, image_end) of the loaded main image (excluding __PAGEZERO).
  static bool image_range(uintptr_t *begin, uintptr_t *end) {
    auto const image = loader::Image::main;
    if (!image) return false;
    uintptr_t lo = UINTPTR_MAX, hi = 0;
    for (auto const &seg : image->object.segments) {
      if (seg.name == "__PAGEZERO" || seg.vmsize == 0) continue;
      auto const b = (uintptr_t)(seg.vmaddr + image->slide);
      if (b < lo) lo = b;
      if (b + seg.vmsize > hi) hi = b + seg.vmsize;
    }
    if (lo >= hi) return false;
    *begin = lo;
    *end = hi;
    return true;
  }

  static int dladdr(const void *addr, Dl_info *info) {
    // WC3's __INIT0 integrity code does dladdr(own_code)->dli_fbase and walks the
    // mach header it finds there, so this must be real for the main image. The header
    // sits at the image base (__TEXT has fileoff 0). Everything else still fails.
    uintptr_t begin = 0, end = 0;
    if (image_range(&begin, &end) && (uintptr_t)addr >= begin && (uintptr_t)addr < end) {
      info->dli_fname = loader::Image::main->path.c_str();
      info->dli_fbase = loader::Image::main->address;
      info->dli_sname = nullptr;
      info->dli_saddr = nullptr;
      return 1;
    }
    return 0;
  }

  static uint32_t dyld_image_count() {
    // Only the main image is a mach-o image; everything else is ELF.
    return 1;
  }

  static const struct mach_header *dyld_get_image_header(uint32_t index) {
    if (index != 0 || !loader::Image::main) return nullptr;
    return (const struct mach_header *)loader::Image::main->address;
  }

  static const char *dyld_get_image_name(uint32_t index) {
    if (index != 0 || !loader::Image::main) return nullptr;
    return loader::Image::main->path.c_str();
  }

  static intptr_t dyld_get_image_vmaddr_slide(uint32_t index) {
    if (index != 0 || !loader::Image::main) return 0;
    return loader::Image::main->slide;
  }

  static void *NSGetMachExecuteHeader() {
    return loader::Image::main ? loader::Image::main->address : nullptr;
  }

  static int dlclose(void *handle) {
    // dont care, allways success
    return 0;
  }

  static const char *dlerror(void) {
    // dont't care
    return "Not implemented!";
  }

  static void *dlopen(const char *path, int mode) {
    if (!path) return nullptr;
    // Whitelist libc.
    if (strstr(path, "libc")) return (void *)'cbil';
    return nullptr;
  }

  static void *dlsym(void *handle, const char *symbol) {
    if (!symbol) return nullptr;
    return lib::resolve(symbol);
  }

  // Everything is prebound, but WC3's __INIT0 integrity pass restores la_symbol_ptr
  // slots to their original __stub_helper entries (to shake off IAT hooks) and then
  // relies on genuine lazy binding. So the binder has to actually work: the helper
  // entry pushed the offset of this import's entry in the lazy bind opcode stream,
  // which read_bindings recorded as Binding::stream_offset.
  extern "C" void* macholo_fast_stub_entry(void* cache, uintptr_t lazy_offset) {
    auto const image = loader::Image::main;
    assert_that(image != nullptr);
    for (auto const& b : image->object.bind_lazy) {
      if (b.stream_offset != lazy_offset) continue;
      auto const p = lib::resolve_import(b.name);
      if (!p) panic("lazy bind: unresolved %s", b.name.c_str());
      auto const target = (uintptr_t)p + b.value;
      *(uintptr_t*)(b.offset + image->slide) = target;
      return (void*)target;
    }
    panic("lazy bind: no binding at stream offset %lx", lazy_offset);
  }

  // Register-preserving front end, mirroring dyld's dyld_stub_binder.s. On entry the
  // stack is [ret-to-caller][lazy offset][cache], so the C resolver's parameters live
  // at 8(%rbp)/16(%rbp) once the frame is set up, and both words are popped before
  // tail-jumping to the freshly bound target.
  __attribute__((naked)) static void dyld_stub_binder() {
    __asm__ volatile(
        "pushq %rbp\n"
        "movq %rsp, %rbp\n"
        "subq $240, %rsp\n"
        "movq %rdi, 0(%rsp)\n"
        "movq %rsi, 8(%rsp)\n"
        "movq %rdx, 16(%rsp)\n"
        "movq %rcx, 24(%rsp)\n"
        "movq %r8, 32(%rsp)\n"
        "movq %r9, 40(%rsp)\n"
        "movq %rax, 48(%rsp)\n"
        "movdqa %xmm0, 64(%rsp)\n"
        "movdqa %xmm1, 80(%rsp)\n"
        "movdqa %xmm2, 96(%rsp)\n"
        "movdqa %xmm3, 112(%rsp)\n"
        "movdqa %xmm4, 128(%rsp)\n"
        "movdqa %xmm5, 144(%rsp)\n"
        "movdqa %xmm6, 160(%rsp)\n"
        "movdqa %xmm7, 176(%rsp)\n"
        "movq 8(%rbp), %rdi\n"   // cache pointer pushed by __stub_helper prologue
        "movq 16(%rbp), %rsi\n"  // lazy bind stream offset pushed by the stub entry
        "call macholo_fast_stub_entry\n"
        "movq %rax, %r11\n"
        "movq 0(%rsp), %rdi\n"
        "movq 8(%rsp), %rsi\n"
        "movq 16(%rsp), %rdx\n"
        "movq 24(%rsp), %rcx\n"
        "movq 32(%rsp), %r8\n"
        "movq 40(%rsp), %r9\n"
        "movq 48(%rsp), %rax\n"
        "movdqa 64(%rsp), %xmm0\n"
        "movdqa 80(%rsp), %xmm1\n"
        "movdqa 96(%rsp), %xmm2\n"
        "movdqa 112(%rsp), %xmm3\n"
        "movdqa 128(%rsp), %xmm4\n"
        "movdqa 144(%rsp), %xmm5\n"
        "movdqa 160(%rsp), %xmm6\n"
        "movdqa 176(%rsp), %xmm7\n"
        "addq $240, %rsp\n"
        "popq %rbp\n"
        "addq $16, %rsp\n"  // drop [lazy offset][cache]
        "jmpq *%r11\n");
  }

  static void tlv_bootstrap(void *block) {
    // NOTE: Should never be called.
    // FIXME: Need to implement TLV first.
    panic("Needs to be replaced by loader and should never be called!");
  }

  static int cxa_atexit(void (*lpfunc)(void *), void *obj, void *lpdso_handle) {
    // FIXME: need proper exit support
    // NOTE: do nothing
    return 0;
  }

  static int atexit(void (*lpfunc)(void)) {
    // FIXME: need proper exit support
    // NOTE: do nothing
    return 0;
  }
}

void *lib::resolve(std::string_view name) {
  static std::pair<std::string_view, void *> OVERRIDE[] = {
#define OVERRIDE(name, into) {name, (void *)into}
      // DATA:
      OVERRIDE("__stderrp", &::stderr),
      OVERRIDE("__stdinp", &::stdin),
      OVERRIDE("__stdoutp", &::stdout),
      OVERRIDE("environ", &::environ),
      // CODE:
      OVERRIDE("get_edata", &stub::get_end),
      OVERRIDE("get_end", &stub::get_etext),
      OVERRIDE("get_etext", &stub::get_edata),
      OVERRIDE("_NSGetExecutablePath", &stub::NSGetExecutablePath),
      OVERRIDE("_dyld_register_func_for_add_image", &stub::dyld_register_func_for_add_image),
      OVERRIDE("_dyld_register_func_for_remove_image", &stub::dyld_register_func_for_remove_image),
      OVERRIDE("_dyld_image_count", &stub::dyld_image_count),
      OVERRIDE("_dyld_get_image_header", &stub::dyld_get_image_header),
      OVERRIDE("_dyld_get_image_name", &stub::dyld_get_image_name),
      OVERRIDE("_dyld_get_image_vmaddr_slide", &stub::dyld_get_image_vmaddr_slide),
      OVERRIDE("_NSGetMachExecuteHeader", &stub::NSGetMachExecuteHeader),
      OVERRIDE("dladdr", &stub::dladdr),
      OVERRIDE("dlclose", &stub::dlclose),
      OVERRIDE("dlerror", &stub::dlerror),
      OVERRIDE("dlopen", &stub::dlopen),
      OVERRIDE("dlsym", &stub::dlsym),
      OVERRIDE("dyld_stub_binder", &stub::dyld_stub_binder),
      OVERRIDE("_tlv_bootstrap", &stub::tlv_bootstrap),
      OVERRIDE("atexit", &stub::atexit),
      OVERRIDE("__cxa_atexit", &stub::cxa_atexit),
#undef OVERRIDE
  };

  if (auto const s = "$darwin$" + std::string(name); auto const p = ::dlsym(nullptr, s.data())) return p;

  static std::string_view BLOCKED[] = {
      "OBJC_METACLASS_$_",
      "OBJC_CLASS_$_",
      "posix_spawn_",
      "exec",
      "_ZNSt3__15mutex",
      "_ZNSt3__115recursive_mutex",
      "_ZNSt3__121recursive_timed_mutex",
      "_ZNSt3__16thread",
      "_ZNSt3__118condition_variable",

  };
  for (auto const [n, p] : OVERRIDE)
    if (n == name) return p;

  for (auto const n : BLOCKED)
    if (name.starts_with(n)) return nullptr;

  return ::dlsym(nullptr, name.data());
}

void *lib::resolve_import(std::string_view name) {
  if (name.starts_with("_")) name = name.substr(1);
  auto const p = lib::resolve(name);
  return p;
}
