#pragma once
// Internal Darwin<->Linux value translation helpers shared by the libsystem shims
// (docs/surface-audit/VERDICTS.md). Hidden: helpers must not join the exported
// stub surface tools/exports-diff.sh guards.
#define STUB_INTERNAL __attribute__((visibility("hidden")))

STUB_INTERNAL int macholo_errno_to_darwin(int linux_errno);
STUB_INTERNAL int macholo_errno_to_linux(int darwin_errno);
STUB_INTERNAL int macholo_signo_to_darwin(int linux_signo);
STUB_INTERNAL int macholo_signo_to_linux(int darwin_signo);

STUB_INTERNAL extern void (*macholo_guest_handlers[64])(int);
STUB_INTERNAL void macholo_sig_trampoline(int linux_signo);
STUB_INTERNAL unsigned macholo_saflags_to_linux(unsigned darwin_flags);
STUB_INTERNAL unsigned macholo_saflags_to_darwin(unsigned linux_flags);
