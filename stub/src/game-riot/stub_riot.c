// Stubs for the bundled Riot frameworks that 16.3+ hard-links via @rpath.
//
//   RPatch.framework  -- the C patcher API (rp_*). During replay playback there is
//                        nothing to patch, so every call reports "success, nothing to
//                        do" and the client's Patching state falls straight through.
//   mvg.framework     -- telemetry/voice metrics (Mvg*). Pure no-ops.
//
// We deliberately do not load these dylibs: they are patcher/telemetry only and pull in
// a large amount of networking that is useless for offline replay.
#include <stddef.h>

#include <common.h>

// ---------------------------------------------------------------------------
// RPatch
// ---------------------------------------------------------------------------

// Opaque handles. The game only ever passes these back to us, so any stable non-NULL
// pointer works -- returning NULL here reads as "out of memory" and trips error paths.
static char rp_settings_handle[1];
static char rp_session_handle[1];
static char rp_canceler_handle[1];

enum { RP_OK = 0 };

void *rp_create_patcher_settings(void) { return rp_settings_handle; }

void rp_destroy_patcher_settings(void *settings) {}

void *rp_create_session(void *settings) { return rp_session_handle; }

void rp_destroy_session(void *session) {}

void *rp_create_canceler(void) { return rp_canceler_handle; }

void rp_destroy_canceler(void *canceler) {}

int rp_add_http_header(void *handle, const char *name, const char *value) { return RP_OK; }

// "Is a patch needed?" -- no, we are already up to date.
int rp_check(void *session, void *canceler, void *out) { return RP_OK; }

// "Apply the patch" -- nothing to apply.
int rp_patch(void *session, void *canceler, void *progress, void *ctx) { return RP_OK; }

void rp_cancel(void *canceler) {}

// No error ever happened; keep these non-NULL so naive strlen/logging paths are safe.
const char *rp_last_error_message(void) { return ""; }

const char *rp_last_error_data(void) { return ""; }

// ---------------------------------------------------------------------------
// mvg (telemetry)
// ---------------------------------------------------------------------------

int MvgInit(void *config) { return 0; }

// Never connected -> the client skips every metric submission path.
int MvgIsConnected(void) { return 0; }

void MvgRegisterLogin(const char *a, const char *b, const char *c) {}

void MvgRegisterLogout(void) {}

// Record counter / record timer.
void MvgRC(const char *name, long long value) {}

void MvgRT(const char *name, long long value) {}
