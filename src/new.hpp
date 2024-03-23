#include <new>
#include <string_view>

namespace newer {
  extern void* (*hook_Znwm)(size_t size);                                                                                 // void* operator new    (size_t size);
  extern void* (*hook_ZnwmRKSt9nothrow_t)(size_t size, std::nothrow_t const& nt);                                         // void* operator new    (size_t size,std::nothrow_t const& nt);
  extern void* (*hook_ZnwmSt11align_val_t)(size_t size, std::align_val_t align);                                          // void* operator new    (size_t size,std::align_val_t align);
  extern void* (*hook_ZnwmSt11align_val_tRKSt9nothrow_t)(size_t size, std::align_val_t align, std::nothrow_t const& nt);  // void* operator new    (size_t size,std::align_val_t align,std::nothrow_t const& nt);
  extern void* (*hook_Znam)(size_t size);                                                                                 // void* operator new[]  (size_t size);
  extern void* (*hook_ZnamRKSt9nothrow_t)(size_t size, std::nothrow_t const& nt);                                         // void* operator new[]  (size_t size,std::nothrow_t const& nt);
  extern void* (*hook_ZnamSt11align_val_t)(size_t size, std::align_val_t align);                                          // void* operator new[]  (size_t size,std::align_val_t align);
  extern void* (*hook_ZnamSt11align_val_tRKSt9nothrow_t)(size_t size, std::align_val_t align, std::nothrow_t const& nt);  // void* operator new[]  (size_t size,std::align_val_t align,std::nothrow_t const& nt);
  extern void (*hook_ZdlPv)(void* ptr);                                                                                   // void operator delete  (void * ptr);
  extern void (*hook_ZdlPvRKSt9nothrow_t)(void* ptr, std::nothrow_t const& nt);                                           // void operator delete  (void * ptr,std::nothrow_t const& nt);
  extern void (*hook_ZdlPvm)(void* ptr, size_t size);                                                                     // void operator delete  (void * ptr,size_t size);
  extern void (*hook_ZdlPvmSt11align_val_t)(void* ptr, size_t size, std::align_val_t align);                              // void operator delete  (void * ptr,size_t size,std::align_val_t align);
  extern void (*hook_ZdlPvSt11align_val_t)(void* ptr, std::align_val_t align);                                            // void operator delete  (void * ptr,std::align_val_t align);
  extern void (*hook_ZdlPvSt11align_val_tRKSt9nothrow_t)(void* ptr, std::align_val_t align, std::nothrow_t const& nt);    // void operator delete  (void * ptr,std::align_val_t align,std::nothrow_t const& nt);
  extern void (*hook_ZdaPv)(void* ptr);                                                                                   // void operator delete[](void * ptr);
  extern void (*hook_ZdaPvRKSt9nothrow_t)(void* ptr, std::nothrow_t const& nt);                                           // void operator delete[](void * ptr,std::nothrow_t const& nt);
  extern void (*hook_ZdaPvm)(void* ptr, size_t size);                                                                     // void operator delete[](void * ptr,size_t size);
  extern void (*hook_ZdaPvmSt11align_val_t)(void* ptr, size_t size, std::align_val_t align);                              // void operator delete[](void * ptr,size_t size,std::align_val_t align);
  extern void (*hook_ZdaPvSt11align_val_t)(void* ptr, std::align_val_t align);                                            // void operator delete[](void * ptr,std::align_val_t align);
  extern void (*hook_ZdaPvSt11align_val_tRKSt9nothrow_t)(void* ptr, std::align_val_t align, std::nothrow_t const& nt);    // void operator delete[](void * ptr,std::align_val_t align,std::nothrow_t const& nt);

  extern bool install_hook(std::string_view name, void* ptr, bool install = true);
}
