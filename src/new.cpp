#include "new.hpp"

#include <cstddef>
#include <cstdlib>

// These are overloaded in following way:
// 1. If there is a "hook" placed take it allways!
// 2. Base "operator new"/"operator delete" are implemented with malloc/free and "operator new align" is implemented with posix_memalign.
// 3. no_throw versions just call throwing versions
// 4. for sized delete call into non sized delete
// 5. for aligned delete call into non aligned delete
// 6. base new/delete [] versions call into non-[] versions
// Branch predictor go brrrrr...

void* (*newer::hook_Znwm)(size_t size) = nullptr;
void* operator new(size_t size) {
  if (newer::hook_Znwm) return newer::hook_Znwm(size);
  return malloc(size);
}

void* (*newer::hook_ZnwmRKSt9nothrow_t)(size_t size, std::nothrow_t const& nt) = nullptr;
void* operator new(size_t size, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZnwmRKSt9nothrow_t) return newer::hook_ZnwmRKSt9nothrow_t(size, nt);
  return operator new(size);
}

void* (*newer::hook_ZnwmSt11align_val_t)(size_t size, std::align_val_t align) = nullptr;
void* operator new(size_t size, std::align_val_t align) {
  if (newer::hook_ZnwmSt11align_val_t) return newer::hook_ZnwmSt11align_val_t(size, align);
  void* ptr = nullptr;
  posix_memalign(&ptr, (size_t)align < 8 ? 8 : (size_t)align, size);
  return ptr;
}

void* (*newer::hook_ZnwmSt11align_val_tRKSt9nothrow_t)(size_t size, std::align_val_t align, std::nothrow_t const& nt) = nullptr;
void* operator new(size_t size, std::align_val_t align, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZnwmSt11align_val_tRKSt9nothrow_t) return newer::hook_ZnwmSt11align_val_tRKSt9nothrow_t(size, align, nt);
  return operator new(size, align);
}

void* (*newer::hook_Znam)(size_t size) = nullptr;
void* operator new[](size_t size) {
  if (newer::hook_Znam) return newer::hook_Znam(size);
  return operator new(size);
}

void* (*newer::hook_ZnamRKSt9nothrow_t)(size_t size, std::nothrow_t const& nt) = nullptr;
void* operator new[](size_t size, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZnamRKSt9nothrow_t) return newer::hook_ZnamRKSt9nothrow_t(size, nt);
  return operator new[](size);
}

void* (*newer::hook_ZnamSt11align_val_t)(size_t size, std::align_val_t align) = nullptr;
void* operator new[](size_t size, std::align_val_t align) {
  if (newer::hook_ZnamSt11align_val_t) return newer::hook_ZnamSt11align_val_t(size, align);
  return operator new(size, align);
}

void* (*newer::hook_ZnamSt11align_val_tRKSt9nothrow_t)(size_t size, std::align_val_t align, std::nothrow_t const& nt) = nullptr;
void* operator new[](size_t size, std::align_val_t align, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZnamSt11align_val_tRKSt9nothrow_t) return newer::hook_ZnamSt11align_val_tRKSt9nothrow_t(size, align, nt);
  return operator new[](size, align);
}

void (*newer::hook_ZdlPv)(void* ptr) = nullptr;
void operator delete(void* ptr) noexcept {
  if (newer::hook_ZdlPv) return newer::hook_ZdlPv(ptr);
  return free(ptr);
}

void (*newer::hook_ZdlPvRKSt9nothrow_t)(void* ptr, std::nothrow_t const& nt) = nullptr;
void operator delete(void* ptr, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZdlPvRKSt9nothrow_t) return newer::hook_ZdlPvRKSt9nothrow_t(ptr, nt);
  return operator delete(ptr);
}

void (*newer::hook_ZdlPvm)(void* ptr, size_t size) = nullptr;
void operator delete(void* ptr, size_t size) noexcept {
  if (newer::hook_ZdlPvm) return newer::hook_ZdlPvm(ptr, size);
  return operator delete(ptr);
}

void (*newer::hook_ZdlPvmSt11align_val_t)(void* ptr, size_t size, std::align_val_t align) = nullptr;
void operator delete(void* ptr, size_t size, std::align_val_t align) noexcept {
  if (newer::hook_ZdlPvmSt11align_val_t) return newer::hook_ZdlPvmSt11align_val_t(ptr, size, align);
  return operator delete(ptr, size);
}

void (*newer::hook_ZdlPvSt11align_val_t)(void* ptr, std::align_val_t align) = nullptr;
void operator delete(void* ptr, std::align_val_t align) noexcept {
  if (newer::hook_ZdlPvSt11align_val_t) return newer::hook_ZdlPvSt11align_val_t(ptr, align);
  return operator delete(ptr);
}

void (*newer::hook_ZdlPvSt11align_val_tRKSt9nothrow_t)(void* ptr, std::align_val_t align, std::nothrow_t const& nt) = nullptr;
void operator delete(void* ptr, std::align_val_t align, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZdlPvSt11align_val_tRKSt9nothrow_t) return newer::hook_ZdlPvSt11align_val_tRKSt9nothrow_t(ptr, align, nt);
  return operator delete(ptr, align);
}

void (*newer::hook_ZdaPv)(void* ptr) = nullptr;
void operator delete[](void* ptr) noexcept {
  if (newer::hook_ZdaPv) return newer::hook_ZdaPv(ptr);
  return operator delete(ptr);
}

void (*newer::hook_ZdaPvRKSt9nothrow_t)(void* ptr, std::nothrow_t const& nt) = nullptr;
void operator delete[](void* ptr, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZdaPvRKSt9nothrow_t) return newer::hook_ZdaPvRKSt9nothrow_t(ptr, nt);
  return operator delete[](ptr);
}

void (*newer::hook_ZdaPvm)(void* ptr, size_t size) = nullptr;
void operator delete[](void* ptr, size_t size) noexcept {
  if (newer::hook_ZdaPvm) return newer::hook_ZdaPvm(ptr, size);
  return operator delete[](ptr);
}

void (*newer::hook_ZdaPvmSt11align_val_t)(void* ptr, size_t size, std::align_val_t align) = nullptr;
void operator delete[](void* ptr, size_t size, std::align_val_t align) noexcept {
  if (newer::hook_ZdaPvmSt11align_val_t) return newer::hook_ZdaPvmSt11align_val_t(ptr, size, align);
  return operator delete[](ptr);
}

void (*newer::hook_ZdaPvSt11align_val_t)(void* ptr, std::align_val_t align) = nullptr;
void operator delete[](void* ptr, std::align_val_t align) noexcept {
  if (newer::hook_ZdaPvSt11align_val_t) return newer::hook_ZdaPvSt11align_val_t(ptr, align);
  return operator delete[](ptr);
}

void (*newer::hook_ZdaPvSt11align_val_tRKSt9nothrow_t)(void* ptr, std::align_val_t align, std::nothrow_t const& nt) = nullptr;
void operator delete[](void* ptr, std::align_val_t align, std::nothrow_t const& nt) noexcept {
  if (newer::hook_ZdaPvSt11align_val_tRKSt9nothrow_t) return newer::hook_ZdaPvSt11align_val_tRKSt9nothrow_t(ptr, align, nt);
  return operator delete[](ptr, align);
}

bool newer::install_hook(std::string_view name, void* ptr, bool install) {
#define CHECK_HOOK(h, n)               \
  if (name == n) {                     \
    if (install) h = (decltype(h))ptr; \
    return true;                       \
  }
  CHECK_HOOK(hook_Znwm, "__Znwm")
  CHECK_HOOK(hook_ZnwmRKSt9nothrow_t, "__ZnwmRKSt9nothrow_t")
  CHECK_HOOK(hook_ZnwmSt11align_val_t, "__ZnwmSt11align_val_t")
  CHECK_HOOK(hook_ZnwmSt11align_val_tRKSt9nothrow_t, "__ZnwmSt11align_val_tRKSt9nothrow_t")
  CHECK_HOOK(hook_Znam, "__Znam")
  CHECK_HOOK(hook_ZnamRKSt9nothrow_t, "__ZnamRKSt9nothrow_t")
  CHECK_HOOK(hook_ZnamSt11align_val_t, "__ZnamSt11align_val_t")
  CHECK_HOOK(hook_ZnamSt11align_val_tRKSt9nothrow_t, "__ZnamSt11align_val_tRKSt9nothrow_t")
  CHECK_HOOK(hook_ZdlPv, "__ZdlPv")
  CHECK_HOOK(hook_ZdlPvRKSt9nothrow_t, "__ZdlPvRKSt9nothrow_t")
  CHECK_HOOK(hook_ZdlPvm, "__ZdlPvm")
  CHECK_HOOK(hook_ZdlPvmSt11align_val_t, "__ZdlPvmSt11align_val_t")
  CHECK_HOOK(hook_ZdlPvSt11align_val_t, "__ZdlPvSt11align_val_t")
  CHECK_HOOK(hook_ZdlPvSt11align_val_tRKSt9nothrow_t, "__ZdlPvSt11align_val_tRKSt9nothrow_t")
  CHECK_HOOK(hook_ZdaPv, "__ZdaPv")
  CHECK_HOOK(hook_ZdaPvRKSt9nothrow_t, "__ZdaPvRKSt9nothrow_t")
  CHECK_HOOK(hook_ZdaPvm, "__ZdaPvm")
  CHECK_HOOK(hook_ZdaPvmSt11align_val_t, "__ZdaPvmSt11align_val_t")
  CHECK_HOOK(hook_ZdaPvSt11align_val_t, "__ZdaPvSt11align_val_t")
  CHECK_HOOK(hook_ZdaPvSt11align_val_tRKSt9nothrow_t, "__ZdaPvSt11align_val_tRKSt9nothrow_t")
  return false;
}
