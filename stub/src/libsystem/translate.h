#pragma once
// Internal Darwin<->Linux value translation helpers shared by the libsystem shims
// (docs/surface-audit/VERDICTS.md). Hidden: helpers must not join the exported
// stub surface tools/exports-diff.sh guards.
#define STUB_INTERNAL __attribute__((visibility("hidden")))

STUB_INTERNAL int macholo_errno_to_darwin(int linux_errno);
STUB_INTERNAL int macholo_errno_to_linux(int darwin_errno);
STUB_INTERNAL int macholo_signo_to_darwin(int linux_signo);
STUB_INTERNAL int macholo_signo_to_linux(int darwin_signo);
