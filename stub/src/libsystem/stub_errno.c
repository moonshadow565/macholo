#include <common.h>
#include <errno.h>
#include <string.h>
#include "translate.h"

// errno policy B (docs/surface-audit/VERDICTS.md §4): the guest reads and writes
// errno exclusively through $darwin$__error() (Darwin's <errno.h> is
// `#define errno (*__error())`), so this one function owns the numbering boundary.
// It returns a thread-local Darwin-numbered slot, refreshed with change detection:
// if glibc's errno moved since we last looked, some forwarded/internal call failed
// and the slot is re-translated; if it did not move, the slot is left alone so
// guest writes (the `errno = 0; call(); if (errno)` idiom) survive.

// Darwin/Linux agree on 1..10 and 12..34; 11 is swapped meaning, 35+ is BSD order.
// Pairs are (darwin, linux); identity for anything not listed.
static const struct { short d, l; } PAIRS[] = {
    {11, 35},   // EDEADLK
    {35, 11},   // EAGAIN / EWOULDBLOCK
    {36, 115},  // EINPROGRESS
    {37, 114},  // EALREADY
    {38, 88},   // ENOTSOCK
    {39, 89},   // EDESTADDRREQ
    {40, 90},   // EMSGSIZE
    {41, 91},   // EPROTOTYPE
    {42, 92},   // ENOPROTOOPT
    {43, 93},   // EPROTONOSUPPORT
    {44, 94},   // ESOCKTNOSUPPORT
    {45, 95},   // ENOTSUP -> EOPNOTSUPP
    {46, 96},   // EPFNOSUPPORT
    {47, 97},   // EAFNOSUPPORT
    {48, 98},   // EADDRINUSE
    {49, 99},   // EADDRNOTAVAIL
    {50, 100},  // ENETDOWN
    {51, 101},  // ENETUNREACH
    {52, 102},  // ENETRESET
    {53, 103},  // ECONNABORTED
    {54, 104},  // ECONNRESET
    {55, 105},  // ENOBUFS
    {56, 106},  // EISCONN
    {57, 107},  // ENOTCONN
    {58, 108},  // ESHUTDOWN
    {59, 109},  // ETOOMANYREFS
    {60, 110},  // ETIMEDOUT
    {61, 111},  // ECONNREFUSED
    {62, 40},   // ELOOP
    {63, 36},   // ENAMETOOLONG
    {64, 112},  // EHOSTDOWN
    {65, 113},  // EHOSTUNREACH
    {66, 39},   // ENOTEMPTY
    {68, 87},   // EUSERS
    {69, 122},  // EDQUOT
    {70, 116},  // ESTALE
    {71, 66},   // EREMOTE
    {77, 37},   // ENOLCK
    {78, 38},   // ENOSYS
    {84, 75},   // EOVERFLOW
    {89, 125},  // ECANCELED
    {90, 43},   // EIDRM
    {91, 42},   // ENOMSG
    {92, 84},   // EILSEQ
    {93, 61},   // ENOATTR -> ENODATA
    {94, 74},   // EBADMSG
    {95, 72},   // EMULTIHOP
    {96, 61},   // ENODATA
    {97, 67},   // ENOLINK
    {98, 63},   // ENOSR
    {99, 60},   // ENOSTR
    {100, 71},  // EPROTO
    {101, 62},  // ETIME
    {102, 95},  // EOPNOTSUPP (POSIX-mode spelling)
    {104, 131}, // ENOTRECOVERABLE
    {105, 130}, // EOWNERDEAD
};
// Darwin-only codes with no Linux number (EPROCLIM, the RPC family, EPWROFF,
// EBADEXEC/EBADARCH/ESHLIBVERS/EBADMACHO, ENOPOLICY, EQFULL) fall through as-is;
// strerror gives them explicit strings below.

int macholo_errno_to_darwin(int e) {
  for (unsigned i = 0; i < sizeof(PAIRS) / sizeof(*PAIRS); i++)
    if (PAIRS[i].l == e) return PAIRS[i].d;
  return e;
}

int macholo_errno_to_linux(int e) {
  for (unsigned i = 0; i < sizeof(PAIRS) / sizeof(*PAIRS); i++)
    if (PAIRS[i].d == e) return PAIRS[i].l;
  return e;
}

int *$darwin$__error(void) {
  static _Thread_local int darwin_errno __attribute__((tls_model("initial-exec")));
  static _Thread_local int last_glibc __attribute__((tls_model("initial-exec")));
  int cur = *__errno_location();
  if (cur != last_glibc) {
    last_glibc = cur;
    darwin_errno = macholo_errno_to_darwin(cur);
  }
  return &darwin_errno;
}

// Guests pass literal Darwin E* constants here, so translate before glibc's
// tables; Darwin-only codes get their real Darwin strings.
static const char *darwin_only_str(int e) {
  switch (e) {
    case 67: return "Too many processes";                     // EPROCLIM
    case 72: return "RPC struct is bad";                      // EBADRPC
    case 73: return "RPC version wrong";                      // ERPCMISMATCH
    case 74: return "RPC prog. not avail";                    // EPROGUNAVAIL
    case 75: return "Program version wrong";                  // EPROGMISMATCH
    case 76: return "Bad procedure for program";              // EPROCUNAVAIL
    case 79: return "Inappropriate file type or format";      // EFTYPE
    case 80: return "Authentication error";                   // EAUTH
    case 81: return "Need authenticator";                     // ENEEDAUTH
    case 82: return "Device power is off";                    // EPWROFF
    case 83: return "Device error";                           // EDEVERR
    case 85: return "Bad executable (or shared library)";     // EBADEXEC
    case 86: return "Bad CPU type in executable";             // EBADARCH
    case 87: return "Shared library version mismatch";        // ESHLIBVERS
    case 88: return "Malformed Mach-o file";                  // EBADMACHO
    case 103: return "Policy not found";                      // ENOPOLICY
    case 106: return "Interface output queue is full";        // EQFULL
    default: return NULL;
  }
}

char *$darwin$strerror(int e) {
  const char *s = darwin_only_str(e);
  if (s) return (char *)s;
  return strerror(macholo_errno_to_linux(e));
}

// Darwin exports the XSI variant (int return, always fills the buffer); glibc's
// dynamic symbol is the GNU char*-returning one, so call glibc's XSI entry point.
extern int __xpg_strerror_r(int e, char *buf, size_t len);

int $darwin$strerror_r(int e, char *buf, size_t len) {
  const char *s = darwin_only_str(e);
  if (s) {
    if (!len) return 34;  // Darwin ERANGE
    strncpy(buf, s, len - 1);
    buf[len - 1] = 0;
    return strlen(s) >= len ? 34 : 0;
  }
  int r = __xpg_strerror_r(macholo_errno_to_linux(e), buf, len);
  return macholo_errno_to_darwin(r);
}
