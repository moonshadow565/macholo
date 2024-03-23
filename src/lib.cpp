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

  static int dladdr(const void *addr, Dl_info *info) {
    // sentry/crashpad bs, allways fail, dont care
    return 0;
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

  static void dyld_stub_binder() {
    // NOTE: Should never be called.
    panic("We dont do lazy binding and prebind all symbols. If you see this, it means a symbol is missing.");
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
