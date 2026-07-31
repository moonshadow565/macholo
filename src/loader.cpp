#include "loader.hpp"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "ak.hpp"
#include "common.hpp"
#include "lib.hpp"
#include "loader.hpp"
#include "macho.hpp"
#include "new.hpp"
#include "objc.hpp"
#include "tlv.hpp"

loader::Image* loader::Image::main = nullptr;

namespace {
  // __PAGEZERO is a 4GB hole with no protections -- never map it.
  bool is_pagezero(macho::Segment const& seg) { return seg.name == "__PAGEZERO"; }

  size_t page_size() {
    static size_t const cached = (size_t)::sysconf(_SC_PAGESIZE);
    return cached;
  }

  uint64_t page_up(uint64_t v) {
    auto const p = (uint64_t)page_size();
    return (v + p - 1) & ~(p - 1);
  }

  // The protection dyld leaves a segment at once fixups are done. SG_READ_ONLY (__DATA_CONST)
  // is declared rw- in initprot but ends up read-only, which is the whole point of the segment.
  int final_prot(macho::Segment const& seg) {
    int prot = PROT_NONE;
    if (seg.initprot & macho::Segment::VM_PROT_READ) prot |= PROT_READ;
    if (seg.initprot & macho::Segment::VM_PROT_WRITE) prot |= PROT_WRITE;
    if (seg.initprot & macho::Segment::VM_PROT_EXECUTE) prot |= PROT_EXEC;
    if (seg.flags & macho::Segment::FLAG_READ_ONLY) prot &= ~PROT_WRITE;
    return prot;
  }

  // Map every segment at vmaddr+slide from its own file offset, with the zerofill tail
  // (vmsize > filesize, i.e. __bss/__common) coming from anonymous pages -- the way dyld
  // does it. Everything is mapped writable on top of its real protections because the
  // loader still has fixups to write (rebase, bind, objc metadata, TLV descriptors);
  // Image::protect() drops them to final_prot() once those are done.
  //
  // Returns the load address, or nullptr if the layout is anything unusual -- the caller
  // then falls back to the previous whole-slice overlay rather than leaving a half-built
  // image behind.
  void* map_segments(int fd, macho::Slice const& sl, macho::Object const& object, uint64_t vm_base, size_t vm_size) {
    auto const page_mask = (uint64_t)page_size() - 1;

    // Validate the whole layout up front, so a bail-out costs nothing.
    for (auto const& seg : object.segments) {
      if (is_pagezero(seg)) continue;
      if (seg.vmaddr < vm_base) return nullptr;
      if (seg.vmaddr + seg.vmsize > vm_base + vm_size) return nullptr;
      if (seg.filesize > seg.vmsize) return nullptr;
      if (seg.vmaddr & page_mask) return nullptr;
      if (seg.filesize && (seg.fileoff & page_mask)) return nullptr;
      if (seg.fileoff + seg.filesize > sl.size) return nullptr;
    }

    // Reserve the whole span first so the segments keep their relative layout even if we
    // don't get the preferred address.
    void* base = ::mmap((void*)vm_base, vm_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (base == MAP_FAILED || base == nullptr) return nullptr;
    auto const slide = (uintptr_t)base - vm_base;

    for (auto const& seg : object.segments) {
      if (is_pagezero(seg) || seg.vmsize == 0) continue;
      int const prot = final_prot(seg) | PROT_WRITE;
      auto const at = (char*)(seg.vmaddr + slide);

      if (seg.filesize != 0) {
        auto const off = (off_t)(sl.offset + seg.fileoff);
        if (::mmap(at, seg.filesize, prot, MAP_PRIVATE | MAP_FIXED, fd, off) == MAP_FAILED) {
          ::munmap(base, vm_size);
          return nullptr;
        }
      }

      // The zerofill tail is still the PROT_NONE reservation; mmap already zeroed the slack
      // in the last file-backed page, so only whole pages past it need opening up.
      auto const tail = page_up(seg.filesize);
      if (seg.vmsize > tail) {
        if (::mprotect(at + tail, (size_t)(seg.vmsize - tail), prot) != 0) {
          ::munmap(base, vm_size);
          return nullptr;
        }
      }
    }

    return base;
  }
}

loader::Image* loader::Image::load(char const* name) {
  unique_fd fd(::open(name, O_RDONLY));
  assert_that(fd.get() != -1);

  size_t size = fd.size();
  assert_that(size != 0);

  auto file = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd.get(), 0);
  assert_that(file != nullptr && (std::intptr_t)file != -1);

  // FAT slice offsets are page-aligned, safe to use as both mmap offset and image base.
  auto const sl = macho::find_slice(file, size);
  auto address = (void*)((char*)file + sl.offset);

  auto object = macho::Object::read(address);

  // NOTE: technically there doesn't have to be a __TEXT segment
  auto const text = object.text_segment();
  assert_that(text != nullptr);
  assert_that(text->fileoff == 0);

  // The image needs more VM than the file slice provides: __DATA's zerofill (__bss) can
  // extend past the end of the slice (16.3: __DATA vmsize 0xf6000 vs filesize 0x1e000).
  // Reserve the whole span anonymously first, then overlay the file-backed part.
  uint64_t vm_end = text->vmaddr + sl.size;
  for (auto const& seg : object.segments) {
    if (seg.name == "__PAGEZERO") continue;
    if (seg.vmaddr + seg.vmsize > vm_end) vm_end = seg.vmaddr + seg.vmsize;
  }
  auto const vm_size = (size_t)(vm_end - text->vmaddr);

  // Try to load at the preferred address, one mapping per segment.
  bool segments_mapped = false;
  if (void* mapped = map_segments(fd.get(), sl, object, text->vmaddr, vm_size)) {
    ::munmap(file, size);
    address = mapped;
    segments_mapped = true;
  } else {
    // Unusual layout: fall back to overlaying the whole file slice on one flat RWX span.
    void* reserved =
        ::mmap((void*)text->vmaddr, vm_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (reserved != nullptr && (std::intptr_t)reserved != -1) {
      void* remaped =
          ::mmap(reserved, sl.size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd.get(), sl.offset);
      if (remaped != nullptr && (std::intptr_t)remaped != -1) {
        ::munmap(file, size);
        address = remaped;
      } else {
        ::munmap(reserved, vm_size);
      }
    }
  }

  auto const slide = (uintptr_t)address - text->vmaddr;
  if (slide != 0) {
    // We have to rebase :(
    for (auto const& r : object.rebase) {
      if (r.type != macho::Rebase::TYPE_POINTER) continue;
      *(uintptr_t*)(r.offset + slide) += slide;
    }
  }

  // Zerofill sections can start before the end of the file-backed part of their segment
  // (16.3: __bss at +0x1d510 of a 0x1e000-long __DATA slice), so the linker's tail padding
  // is what backs them -- zero it explicitly instead of trusting the file.
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
      .segments_mapped = segments_mapped,
      .object = std::move(object),
  };
}

void loader::Image::protect() {
  // Only meaningful when load() actually got one mapping per segment -- the fallback path
  // is a single flat RWX span whose segment boundaries don't line up with real mappings.
  if (!segments_mapped) return;

  for (auto const& seg : object.segments) {
    if (is_pagezero(seg) || seg.vmsize == 0) continue;
    if (::mprotect((void*)(seg.vmaddr + slide), (size_t)seg.vmsize, final_prot(seg)) != 0) {
      fprintf(stderr, "warning: mprotect(%s) failed: %s\n", seg.name.c_str(), strerror(errno));
    }
  }
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
  for (auto const& func : find_section<void (*)(void)>("__mod_init_func")) {
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
