// IOKit. WC3 uses it for exactly one thing that matters at startup: walking the
// IORegistry for an "IOAccelerator" service to identify the GPU (name, VRAM size,
// vendor/device id). There is no IORegistry on Linux, so the registry is modelled as
// empty -- matching succeeds, the iterator is immediately exhausted, and the game falls
// back to its "unknown graphics device" path instead of dereferencing a NULL match
// dictionary.
#include <stdint.h>

#include <common.h>

typedef int kern_return_t;
typedef unsigned mach_port_t;
typedef mach_port_t io_object_t;
typedef io_object_t io_iterator_t;
typedef io_object_t io_registry_entry_t;
typedef io_object_t io_service_t;
typedef io_object_t io_connect_t;
typedef uint32_t IOOptionBits;
typedef const void *CFTypeRef;
typedef const void *CFStringRef;
typedef const void *CFMutableDictionaryRef;
typedef const void *CFAllocatorRef;
typedef uint64_t io_name_t_dummy;

enum { kIOReturnSuccess = 0, kIOReturnNoDevice = 0xe00002c0, kIOReturnUnsupported = 0xe00002c7 };

// DATA: the default IOKit master port. Zero is the real value on macOS too.
mach_port_t kIOMasterPortDefault = 0;

kern_return_t IOMasterPort(mach_port_t bootstrapPort, mach_port_t *masterPort) {
  if (masterPort) *masterPort = kIOMasterPortDefault;
  return kIOReturnSuccess;
}

// A matching dictionary is opaque to the caller -- it only ever hands it straight back
// to IOServiceGetMatchingServices, which finds nothing regardless. NULL would be read
// as "out of memory" and is not safe to return.
static char io_matching_dictionary[1];

CFMutableDictionaryRef IOServiceMatching(const char *name) {
  return (CFMutableDictionaryRef)io_matching_dictionary;
}

// An iterator that is already exhausted.
kern_return_t IOServiceGetMatchingServices(mach_port_t masterPort, CFMutableDictionaryRef matching,
                                           io_iterator_t *existing) {
  if (existing) *existing = 0;
  return kIOReturnSuccess;
}

io_object_t IOIteratorNext(io_iterator_t iterator) {
  return 0;
}

kern_return_t IOObjectRelease(io_object_t object) {
  return kIOReturnSuccess;
}

// ---------------------------------------------------------------------------
// Registry traversal -- unreachable while the iterator above stays empty, but the
// game's error paths still call these with a zero entry.
// ---------------------------------------------------------------------------

io_registry_entry_t IORegistryEntryFromPath(mach_port_t masterPort, const char *path) {
  return 0;
}

kern_return_t IORegistryEntryGetName(io_registry_entry_t entry, char *name) {
  if (name) name[0] = 0;
  return kIOReturnNoDevice;
}

kern_return_t IORegistryEntryGetParentEntry(io_registry_entry_t entry, const char *plane,
                                            io_registry_entry_t *parent) {
  if (parent) *parent = 0;
  return kIOReturnNoDevice;
}

kern_return_t IORegistryEntryGetChildIterator(io_registry_entry_t entry, const char *plane, io_iterator_t *iterator) {
  if (iterator) *iterator = 0;
  return kIOReturnNoDevice;
}

kern_return_t IORegistryEntryGetRegistryEntryID(io_registry_entry_t entry, uint64_t *entryID) {
  if (entryID) *entryID = 0;
  return kIOReturnNoDevice;
}

CFTypeRef IORegistryEntryCreateCFProperty(io_registry_entry_t entry, CFStringRef key, CFAllocatorRef allocator,
                                          IOOptionBits options) {
  return NULL;
}

// ---------------------------------------------------------------------------
// User clients and HID parameters -- mouse acceleration and the like.
// ---------------------------------------------------------------------------

kern_return_t IOServiceOpen(io_service_t service, mach_port_t owningTask, uint32_t type, io_connect_t *connect) {
  if (connect) *connect = 0;
  return kIOReturnNoDevice;
}

kern_return_t IOServiceClose(io_connect_t connect) {
  return kIOReturnSuccess;
}

kern_return_t IOConnectGetService(io_connect_t connect, io_service_t *service) {
  if (service) *service = 0;
  return kIOReturnNoDevice;
}

kern_return_t IOHIDGetAccelerationWithKey(io_connect_t handle, CFStringRef key, double *acceleration) {
  if (acceleration) *acceleration = 1.0;
  return kIOReturnUnsupported;
}

kern_return_t IOHIDSetAccelerationWithKey(io_connect_t handle, CFStringRef key, double acceleration) {
  return kIOReturnUnsupported;
}

kern_return_t IOHIDSetCFTypeParameter(io_connect_t handle, CFStringRef key, CFTypeRef parameter) {
  return kIOReturnUnsupported;
}

void *IOHIDEventSystemClientCreateSimpleClient(CFAllocatorRef allocator) {
  return NULL;
}

CFTypeRef IOHIDEventSystemClientCopyProperty(void *client, CFStringRef key) {
  return NULL;
}
