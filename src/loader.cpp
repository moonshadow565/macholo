#define _GNU_SOURCE 1
#include "loader.hpp"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

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

  // True if every segment sits page-aligned inside [vm_base, vm_base+vm_size) with its
  // file part inside the slice -- i.e. the layout the mapping code below assumes.
  bool layout_is_ordinary(macho::Slice const& sl, macho::Object const& object, uint64_t vm_base, size_t vm_size) {
    auto const page_mask = (uint64_t)page_size() - 1;
    for (auto const& seg : object.segments) {
      if (is_pagezero(seg)) continue;
      if (seg.vmaddr < vm_base) return false;
      if (seg.vmaddr + seg.vmsize > vm_base + vm_size) return false;
      if (seg.filesize > seg.vmsize) return false;
      if (seg.vmaddr & page_mask) return false;
      if (seg.filesize && (seg.fileoff & page_mask)) return false;
      if (seg.fileoff + seg.filesize > sl.size) return false;
    }
    return true;
  }

  // Build the image's backing store: an anonymous shared file laid out by *VM address*,
  // not by file offset. Offset (vmaddr - vm_base) in the memfd is the byte living at
  // vmaddr, segment file data copied in and zerofill left zero.
  //
  // Laying it out this way is what makes mach_vm_remap able to hand out a genuine
  // second view of any part of the image (WC3's packer decrypts __TEXT through such an
  // alias, and asks for the whole image in one go, spanning several segments). A memfd
  // laid out by file offset could not serve that: __LINKEDIT's file offset and its VM
  // offset differ once an earlier segment has a zerofill tail.
  //
  // Returns -1 if the file cannot be built.
  int make_backing(void const* file, macho::Slice const& sl, macho::Object const& object, uint64_t vm_base,
                   size_t vm_size) {
    int const fd = ::memfd_create("macholo-image", MFD_CLOEXEC);
    if (fd == -1) return -1;
    if (::ftruncate(fd, (off_t)vm_size) != 0) {
      ::close(fd);
      return -1;
    }
    for (auto const& seg : object.segments) {
      if (is_pagezero(seg) || seg.filesize == 0) continue;
      auto const src = (char const*)file + sl.offset + seg.fileoff;
      if (::pwrite(fd, src, seg.filesize, (off_t)(seg.vmaddr - vm_base)) != (ssize_t)seg.filesize) {
        ::close(fd);
        return -1;
      }
    }
    return fd;
  }

  // Map every segment at vmaddr+slide from the backing file built above -- one mapping
  // per segment, covering the zerofill tail (__bss/__common) too, which is already zero
  // there. Everything is mapped writable on top of its real protections because the
  // loader still has fixups to write (rebase, bind, objc metadata, TLV descriptors);
  // Image::protect() drops them to final_prot() once those are done.
  //
  // Returns the load address, or nullptr if anything fails -- the caller then falls back
  // to the previous whole-slice overlay rather than leaving a half-built image behind.
  void* map_segments(int fd, macho::Object const& object, uint64_t vm_base, size_t vm_size) {
    // Reserve the whole span first so the segments keep their relative layout even if we
    // don't get the preferred address.
    void* base = ::mmap((void*)vm_base, vm_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (base == MAP_FAILED || base == nullptr) return nullptr;
    auto const slide = (uintptr_t)base - vm_base;

    for (auto const& seg : object.segments) {
      if (is_pagezero(seg) || seg.vmsize == 0) continue;
      int const prot = final_prot(seg) | PROT_WRITE;
      auto const at = (char*)(seg.vmaddr + slide);
      auto const off = (off_t)(seg.vmaddr - vm_base);

      // MAP_SHARED, not MAP_PRIVATE: the backing file is our own memfd, so this is just
      // as private to the process, but it lets a second mapping of the same offsets
      // alias these pages (see lib::stub::mach_vm_remap).
      if (::mmap(at, page_up(seg.vmsize), prot, MAP_SHARED | MAP_FIXED, fd, off) == MAP_FAILED) {
        ::munmap(base, vm_size);
        return nullptr;
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

  // Deliberately never closed: the mappings outlive load(), and mach_vm_remap needs it.
  int backing = layout_is_ordinary(sl, object, text->vmaddr, vm_size)
                    ? make_backing(file, sl, object, text->vmaddr, vm_size)
                    : -1;

  // Try to load at the preferred address, one mapping per segment.
  bool segments_mapped = false;
  if (void* mapped = backing != -1 ? map_segments(backing, object, text->vmaddr, vm_size) : nullptr) {
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
      .backing_fd = segments_mapped ? backing : -1,
      .backing_size = segments_mapped ? vm_size : 0,
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

namespace {
  // Darwin comm page. macOS pins a kernel-shared page at a fixed virtual address and
  // inlines mach_absolute_time() as a direct read of it rather than a call -- so no import
  // stub can intercept it. WC3's obfuscated timing/anti-tamper routine does exactly this
  // (movabs $0x7fffffe00068 -> ...), faulting because macholo never maps the page.
  //
  // The read has two arms, keyed on the "approximate time supported" byte at +0x88:
  //  - supported  -> take the fast path: return the u64 at +0x80 (a mach time the kernel
  //                  bumps continuously); this is the arm real hardware uses.
  //  - unsupported-> fall back to a rdtsc+scale/shift/base computation.
  // We enable the fast path and keep +0x80 advancing from CLOCK_MONOTONIC via a tiny
  // updater thread, so the game sees a real, monotonically increasing clock.
  constexpr uintptr_t COMM_PAGE_BASE = 0x00007fffffe00000ull;
  constexpr size_t COMM_PAGE_SIZE = 0x1000;
  constexpr uintptr_t COMM_PAGE_APPROX_TIME = COMM_PAGE_BASE + 0x80;
  constexpr uintptr_t COMM_PAGE_APPROX_TIME_SUPPORTED = COMM_PAGE_BASE + 0x88;

  uint64_t monotonic_ns() {
    struct timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
  }

  void* comm_page_updater(void*) {
    for (;;) {
      *(volatile uint64_t*)COMM_PAGE_APPROX_TIME = monotonic_ns();
      struct timespec nap = {0, 250000};  // 250us
      ::nanosleep(&nap, nullptr);
    }
    return nullptr;
  }

  void map_comm_page() {
    void* p = ::mmap((void*)COMM_PAGE_BASE, COMM_PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
      fprintf(stderr, "warning: could not map comm page at %p: %s\n", (void*)COMM_PAGE_BASE, strerror(errno));
      return;
    }
    auto at = [&](uintptr_t off) { return (char*)COMM_PAGE_BASE + off; };
    // Also fill the rdtsc-path fields (scale=2^32-1, shift=0, generation=1) so that arm,
    // if ever taken, still reads a sane, monotonic value rather than garbage.
    *(uint64_t*)at(0x50) = 0;                    // tsc_base
    *(uint32_t*)at(0x58) = 0xffffffffu;          // scale
    *(uint32_t*)at(0x5c) = 0;                    // shift
    *(uint64_t*)at(0x60) = 0;                    // ns_base
    *(uint32_t*)at(0x68) = 1;                    // generation (nonzero, stable)
    *(uint64_t*)at(0x80) = monotonic_ns();       // approximate mach time
    *(uint8_t*)at(0x88) = 1;                     // approximate time supported -> fast path
    // Page must stay writable for the updater thread; the game only ever reads it.
    pthread_t tid;
    if (pthread_create(&tid, nullptr, comm_page_updater, nullptr) == 0) pthread_detach(tid);
  }
}  // namespace

int loader::Image::run(int argc, char** argv, char** env) {
  const auto do_debug_break = !!getenv("MACHOLO_DEBUG_BREAK");
  map_comm_page();
  GSInitializeProcess(argc, argv, env);

  auto const start = (int (*)(int argc, char** argv, char** env))(object.entry->offset + object.text_segment()->vmaddr + slide);
  if (do_debug_break)
    debug_break();
  return start(argc, argv, env);
}
