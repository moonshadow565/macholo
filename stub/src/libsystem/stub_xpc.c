#include <stdbool.h>
#include <stdint.h>

#include <common.h>

// dyld's @available() backend, not libxpc — it is only filed here because the audit
// (docs/surface-audit/libsystem.tsv) attributes it to libxpc. Clang emits a call to this
// for every `if (@available(macOS X, *))`; 16.15's Metal path is the first code in any
// audited version that actually reaches one.
//
// The answer must agree with the rest of macholo's idea of what OS this is, which
// stub_sys.c pins at 10.15.6 via kern.osproductversion.
#define MACHOLO_PLATFORM_MACOS 1
#define MACHOLO_MACOS_VERSION ((10u << 16) | (15u << 8) | 6u)

typedef struct {
  uint32_t platform;
  uint32_t version;  // xxxx.yy.zz packed as (major << 16) | (minor << 8) | patch
} dyld_build_version_t;

bool _availability_version_check(uint32_t count, dyld_build_version_t versions[]) {
  for (uint32_t i = 0; i < count; ++i) {
    if (versions[i].platform != MACHOLO_PLATFORM_MACOS) continue;
    return versions[i].version <= MACHOLO_MACOS_VERSION;
  }
  // No clause names macOS: dyld treats a platform it is not running as unavailable.
  return false;
}

void bootstrap_look_up() {
    // TODO
    panic("bootstrap_look_up");
}

void bootstrap_strerror() {
    // TODO
    panic("bootstrap_strerror");
}
