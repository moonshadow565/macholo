#include <errno.h>
#include <malloc.h>
#include <pwd.h>
#include <stdint.h>
#include <string.h>

#include <common.h>

extern int $darwin$sysctlbyname(const char *name, void *oldp, size_t *oldlenp, const void *newp, size_t newlen);

struct $darwin$passwd {
  char *pw_name;    /* user name */
  char *pw_passwd;  /* encrypted password */
  uid_t pw_uid;     /* user uid */
  gid_t pw_gid;     /* user gid */
  time_t pw_change; /* password change time */
  char *pw_class;   /* user access class */
  char *pw_gecos;   /* Honeywell login info */
  char *pw_dir;     /* home directory */
  char *pw_shell;   /* default shell */
  time_t pw_expire; /* account expiration */
};

struct $darwin$passwd *$darwin$getpwuid(uid_t uid) {
  // TODO
  panic("getpwuid");
  static struct $darwin$passwd lazy = {0};
  int inited = 0;
  if (inited == 0) {
    struct passwd *gnu = getpwuid(uid);
    if (gnu == NULL) return NULL;
    lazy.pw_name = gnu->pw_name;
    lazy.pw_passwd = gnu->pw_passwd;
    lazy.pw_uid = gnu->pw_uid;
    lazy.pw_gid = gnu->pw_gid;
    lazy.pw_gecos = gnu->pw_gecos;
    lazy.pw_dir = gnu->pw_dir;
    lazy.pw_shell = gnu->pw_shell;
    inited = 1;
  }
  return &lazy;
}

// backtrace_from_fp(void *startfp, void **array, int size) -- crash reporting only.
// We never produce Darwin backtraces, so report zero frames.
int backtrace_from_fp(void *startfp, void **array, int size) {
  return 0;
}

// ---------------------------------------------------------------------------
// Odds and ends WC3 imports from libSystem.
// ---------------------------------------------------------------------------

// The legacy raw sysctl(2) MIB interface. Only the handful of MIBs anyone still asks
// for are recognised; everything else reports "no such node" like a real kernel.
#define $darwin$CTL_KERN 1
#define $darwin$CTL_HW 6
#define $darwin$KERN_PROC 14
#define $darwin$KERN_PROC_PID 1
#define $darwin$HW_NCPU 3
#define $darwin$HW_MEMSIZE 24

int $darwin$sysctl(int *name, unsigned namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen) {
  if (!name || namelen < 2) {
    errno = EINVAL;
    return -1;
  }
  if (name[0] == $darwin$CTL_HW && name[1] == $darwin$HW_NCPU)
    return $darwin$sysctlbyname("hw.ncpu", oldp, oldlenp, newp, newlen);
  if (name[0] == $darwin$CTL_HW && name[1] == $darwin$HW_MEMSIZE)
    return $darwin$sysctlbyname("hw.memsize", oldp, oldlenp, newp, newlen);
  // KERN_PROC/KERN_PROC_PID is the "am I being debugged?" probe (it reads P_TRACED out
  // of the returned kinfo_proc). Report a zeroed record: not traced.
  if (name[0] == $darwin$CTL_KERN && name[1] == $darwin$KERN_PROC) {
    if (oldp && oldlenp) memset(oldp, 0, *oldlenp);
    return 0;
  }
  fprintf(stderr, "[sysctl] unhandled mib: %d.%d\n", name[0], name[1]);
  errno = ENOENT;
  return -1;
}

void $darwin$uuid_clear(uint8_t uuid[16]) {
  if (uuid) memset(uuid, 0, 16);
}

size_t $darwin$malloc_size(void const *ptr) {
  return ptr ? malloc_usable_size((void *)ptr) : 0;
}

char ***$darwin$_NSGetEnviron(void) {
  extern char **environ;
  return &environ;
}

// Darwin's coarse clock. mach_absolute_time already reports nanoseconds here
// (stub_kernel.c), so the "approximate" variant is the same number.
uint64_t $darwin$mach_approximate_time(void) {
  extern uint64_t mach_absolute_time(void);
  return mach_absolute_time();
}

int $darwin$proc_name(int pid, void *buffer, uint32_t buffersize) {
  if (!buffer || !buffersize) return 0;
  char const *name = "Warcraft III";
  size_t n = strlen(name);
  if (n >= buffersize) n = buffersize - 1;
  memcpy(buffer, name, n);
  ((char *)buffer)[n] = 0;
  return (int)n;
}

int32_t $darwin$NSVersionOfRunTimeLibrary(char const *libraryName) {
  // "That library is not loaded." Callers use this for feature detection and take the
  // conservative branch on -1.
  return -1;
}

void $darwin$DebugStr(unsigned char const *str) {
  if (str) fprintf(stderr, "[DebugStr] %.*s\n", (int)str[0], (char const *)str + 1);
}
