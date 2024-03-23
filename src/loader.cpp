#include "loader.hpp"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>

#include "ak.hpp"
#include "common.hpp"
#include "lib.hpp"
#include "loader.hpp"
#include "macho.hpp"
#include "new.hpp"
#include "objc.hpp"
#include "tlv.hpp"

loader::Image* loader::Image::main = nullptr;

loader::Image* loader::Image::load(char const* name) {
  unique_fd fd(::open(name, O_RDONLY));
  assert_that(fd.get() != -1);

  size_t size = fd.size();
  assert_that(size != 0);

  auto address = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd.get(), 0);
  assert_that(address != nullptr && (std::intptr_t)address != -1);

  auto object = macho::Object::read(address);

  // NOTE: technically there doesn't have to be a __TEXT segment
  auto const text = object.text_segment();
  assert_that(text != nullptr);
  assert_that(text->fileoff == 0);

  // Try to load at prefered addres...
  // FIXME: this is horibly wrong.
  void* remaped = ::mmap((void*)text->vmaddr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd.get(), 0);
  if (remaped != nullptr && (std::intptr_t)remaped != -1) {
    ::munmap(address, size);
    address = remaped;
  }

  auto const slide = (uintptr_t)address - text->vmaddr;
  if (slide != 0) {
    // We have to rebase :(
    for (auto const& r : object.rebase) {
      if (r.type != macho::Rebase::TYPE_POINTER) continue;
      *(uintptr_t*)(r.offset + slide) += slide;
    }
  }

  // NOTE: this is not really correct...
  // FIXME: this is horibly wrong, we should map each section properly instead of doing this.
  for (auto const& seg : object.segments) {
    if (seg.filesize == 0) continue;
    for (auto const& s : seg.sections) {
      if (s.type == macho::Section::TYPE_ZEROFILL || s.type == macho::Section::TYPE_THREAD_LOCAL_ZEROFILL) {
        ::memset((void*)(slide + s.addr), 0, s.size);
      }
    }
  }

  return new loader::Image{
      .path = std::string(name),
      .address = address,
      .slide = slide,
      .object = std::move(object),
  };
}

void loader::Image::bind() {
  for (auto const& s : object.bind) {
    if (s.type != macho::Binding::TYPE_POINTER) continue;
    if (objc::is_objc_name(s.name)) continue;
    auto const p = lib::resolve_import(s.name);
    if (!p) warn_unresolved("unresolved at %lx %s +%lx\n", s.offset, s.name.c_str(), s.value);
    if (!p) continue;
    *(uintptr_t*)(s.offset + slide) = (uintptr_t)p + s.value;
  }

  for (auto const& s : object.bind_lazy) {
    if (s.type != macho::Binding::TYPE_POINTER) continue;
    if (objc::is_objc_name(s.name)) continue;
    auto const p = lib::resolve_import(s.name);
    if (!p) warn_unresolved("unresolved at %lx %s +%lx\n", s.offset, s.name.c_str(), s.value);
    if (!p) continue;
    *(uintptr_t*)(s.offset + slide) = (uintptr_t)p + s.value;
  }

  for (auto const& e : object.exports) {
    if (newer::install_hook(e.name, nullptr, false)) {
      newer::install_hook(e.name, (char*)address + e.symbol_offset.value(), true);
    }
  }

  objc::fixup_image(this);
  tlv::fixup_image(this);
}

void loader::Image::init() {
  for (auto const& func : find_data_section<void (*)(void)>("__mod_init_func")) {
    (*func)();
  }
  ak::patch_sink(this);
}

extern "C" void GSInitializeProcess(int argc, char **argv, char **envp);

int loader::Image::run(int argc, char** argv, char** env) {
  const auto do_debug_break = !!getenv("MACHOLO_DEBUG_BREAK");
  GSInitializeProcess(argc, argv, env);

  auto const start = (int (*)(int argc, char** argv, char** env))(object.entry->offset + object.text_segment()->vmaddr + slide);
  if (do_debug_break)
    debug_break();
  return start(argc, argv, env);
}
