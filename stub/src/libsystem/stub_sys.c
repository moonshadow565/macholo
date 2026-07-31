#include <errno.h>
#include <error.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

#include <common.h>
#include "translate.h"

struct $darwin$utsname {
  char sysname[256];
  char nodename[256];
  char release[256];
  char version[256];
  char machine[256];
};

struct $darwin$sigaction {
  void *sigaction_u;
  int sa_flags;
  uint32_t sa_mask;
};

void arc4random_buf(void *buf, size_t nbytes) {
  // chosen by fair dice roll.
  // guaranteed to be random.
  memset(buf, 4, nbytes);
}

int $darwin$sigaction(int dsig, const struct $darwin$sigaction *act, struct $darwin$sigaction *oact) {
  // Track 3 follow-up (VERDICTS.md par.9): signo + SA_* flags are translated and
  // simple handlers are really installed through the stub_signal.c trampoline
  // (guest handlers expect Darwin signal numbers). SA_SIGINFO (Darwin 0x40)
  // handlers stay refused: they would receive a Linux-layout siginfo_t, which is
  // garbage to Darwin-compiled code -- log and pretend success, as before.
  int lsig = macholo_signo_to_linux(dsig);
  if (lsig <= 0 || lsig >= 64) {
    *__errno_location() = EINVAL;
    return -1;
  }

  struct sigaction cur;
  if (sigaction(lsig, NULL, &cur) != 0) return -1;
  void (*prev_guest)(int) = macholo_guest_handlers[lsig];

  if (oact) {
    oact->sigaction_u = prev_guest             ? (void *)prev_guest
                        : cur.sa_handler == SIG_DFL || cur.sa_handler == SIG_IGN ? (void *)cur.sa_handler
                                                                                 : (void *)SIG_DFL;
    oact->sa_flags = (int)macholo_saflags_to_darwin((unsigned)cur.sa_flags);
    oact->sa_mask = 0;
  }
  if (!act) return 0;

  if (act->sa_flags & 0x40) {  // SA_SIGINFO
    fprintf(stderr, "ignoring SA_SIGINFO sigaction: darwin sig %d\n", dsig);
    return 0;
  }

  struct sigaction gnu = {0};
  gnu.sa_flags = (int)macholo_saflags_to_linux((unsigned)act->sa_flags);
  sigemptyset(&gnu.sa_mask);
  void (*h)(int) = (void (*)(int))act->sigaction_u;
  if (h == SIG_DFL || h == SIG_IGN) {
    macholo_guest_handlers[lsig] = NULL;
    gnu.sa_handler = h;
  } else {
    macholo_guest_handlers[lsig] = h;
    gnu.sa_handler = macholo_sig_trampoline;
  }
  return sigaction(lsig, &gnu, NULL);
}

// Answers a sysctl query following the BSD contract: with oldp == NULL just report the
// size, otherwise copy (failing with ENOMEM if the caller's buffer is too small).
static int sysctl_return(void *oldp, size_t *oldlenp, void const *value, size_t size) {
  if (!oldp) {
    *oldlenp = size;
    return 0;
  }
  if (*oldlenp < size) {
    *oldlenp = size;
    errno = ENOMEM;
    return -1;
  }
  memcpy(oldp, value, size);
  *oldlenp = size;
  return 0;
}

static int sysctl_return_i32(void *oldp, size_t *oldlenp, int32_t value) {
  return sysctl_return(oldp, oldlenp, &value, sizeof(value));
}

static int sysctl_return_i64(void *oldp, size_t *oldlenp, int64_t value) {
  return sysctl_return(oldp, oldlenp, &value, sizeof(value));
}

static int sysctl_return_str(void *oldp, size_t *oldlenp, char const *value) {
  return sysctl_return(oldp, oldlenp, value, strlen(value) + 1);
}

// Pulls a "key\t: value" line out of /proc/cpuinfo.
static char const *cpu_brand_string(void) {
  static char brand[256] = {0};
  if (brand[0]) return brand;
  strcpy(brand, "Unknown CPU");
  FILE *f = fopen("/proc/cpuinfo", "r");
  if (f) {
    char line[512];
    while (fgets(line, sizeof(line), f)) {
      if (strncmp(line, "model name", 10) != 0) continue;
      char *colon = strchr(line, ':');
      if (!colon) continue;
      for (++colon; *colon == ' '; ++colon) {}
      size_t len = strlen(colon);
      while (len && (colon[len - 1] == '\n' || colon[len - 1] == ' ')) colon[--len] = '\0';
      if (len) {
        snprintf(brand, sizeof(brand), "%s", colon);
      }
      break;
    }
    fclose(f);
  }
  return brand;
}

static int64_t host_memsize(void) {
  long const pages = sysconf(_SC_PHYS_PAGES);
  long const page_size = sysconf(_SC_PAGESIZE);
  if (pages > 0 && page_size > 0) return (int64_t)pages * (int64_t)page_size;
  return (int64_t)8 * 1024 * 1024 * 1024;
}

static int32_t host_ncpu(void) {
  long const n = sysconf(_SC_NPROCESSORS_ONLN);
  return n > 0 ? (int32_t)n : 1;
}

// NOTE: this used to fail unconditionally, which told the client it had 0 CPUs and 0
// bytes of RAM ("CPU: , (0 cores) ... Physical memory: 0" in the game log). Anything
// that sizes a worker pool or a streaming budget off those numbers then degenerates.
int $darwin$sysctlbyname(const char *name, void *oldp, size_t *oldlenp, const void *newp, size_t newlen) {
  if (!name || !oldlenp) {
    errno = EINVAL;
    return -1;
  }
  // Writes are not supported, but pretend they worked -- these are all tunables we don't model.
  if (newp || newlen) return 0;

  // --- hw ---
  if (!strcmp(name, "hw.ncpu") || !strcmp(name, "hw.activecpu") || !strcmp(name, "hw.logicalcpu") ||
      !strcmp(name, "hw.logicalcpu_max") || !strcmp(name, "hw.availcpu"))
    return sysctl_return_i32(oldp, oldlenp, host_ncpu());
  if (!strcmp(name, "hw.physicalcpu") || !strcmp(name, "hw.physicalcpu_max"))
    return sysctl_return_i32(oldp, oldlenp, host_ncpu());
  if (!strcmp(name, "hw.packages")) return sysctl_return_i32(oldp, oldlenp, 1);
  if (!strcmp(name, "hw.memsize")) return sysctl_return_i64(oldp, oldlenp, host_memsize());
  if (!strcmp(name, "hw.pagesize") || !strcmp(name, "hw.pagesize32"))
    return sysctl_return_i32(oldp, oldlenp, (int32_t)sysconf(_SC_PAGESIZE));
  if (!strcmp(name, "hw.cachelinesize")) return sysctl_return_i64(oldp, oldlenp, 64);
  if (!strcmp(name, "hw.l1icachesize") || !strcmp(name, "hw.l1dcachesize"))
    return sysctl_return_i64(oldp, oldlenp, 32 * 1024);
  if (!strcmp(name, "hw.l2cachesize")) return sysctl_return_i64(oldp, oldlenp, 512 * 1024);
  if (!strcmp(name, "hw.l3cachesize")) return sysctl_return_i64(oldp, oldlenp, 8 * 1024 * 1024);
  if (!strcmp(name, "hw.cpufrequency") || !strcmp(name, "hw.cpufrequency_max"))
    return sysctl_return_i64(oldp, oldlenp, 3000000000LL);
  if (!strcmp(name, "hw.busfrequency")) return sysctl_return_i64(oldp, oldlenp, 100000000LL);
  if (!strcmp(name, "hw.byteorder")) return sysctl_return_i32(oldp, oldlenp, 1234);
  if (!strcmp(name, "hw.machine")) return sysctl_return_str(oldp, oldlenp, "x86_64");
  if (!strcmp(name, "hw.model")) return sysctl_return_str(oldp, oldlenp, "MacPro7,1");
  if (!strncmp(name, "hw.optional.", 12)) {
    // CPU feature bits: claim the SSE/AVX baseline any x86_64 host has, deny the rest.
    char const *feature = name + 12;
    int const have = !strcmp(feature, "sse") || !strcmp(feature, "sse2") || !strcmp(feature, "sse3") ||
                     !strcmp(feature, "supplementalsse3") || !strcmp(feature, "sse4_1") ||
                     !strcmp(feature, "sse4_2") || !strcmp(feature, "floatingpoint") || !strcmp(feature, "x86_64");
    return sysctl_return_i32(oldp, oldlenp, have);
  }

  // --- machdep ---
  if (!strcmp(name, "machdep.cpu.brand_string")) return sysctl_return_str(oldp, oldlenp, cpu_brand_string());
  if (!strcmp(name, "machdep.cpu.vendor")) return sysctl_return_str(oldp, oldlenp, "GenuineIntel");
  if (!strcmp(name, "machdep.cpu.core_count") || !strcmp(name, "machdep.cpu.thread_count"))
    return sysctl_return_i32(oldp, oldlenp, host_ncpu());

  // --- kern ---
  if (!strcmp(name, "kern.ostype")) return sysctl_return_str(oldp, oldlenp, "Darwin");
  if (!strcmp(name, "kern.osrelease")) return sysctl_return_str(oldp, oldlenp, "19.6.0");
  if (!strcmp(name, "kern.osversion")) return sysctl_return_str(oldp, oldlenp, "19G73");
  if (!strcmp(name, "kern.osproductversion")) return sysctl_return_str(oldp, oldlenp, "10.15.6");
  if (!strcmp(name, "kern.hostname")) return sysctl_return_str(oldp, oldlenp, "macholo");
  if (!strcmp(name, "kern.version"))
    return sysctl_return_str(oldp, oldlenp, "Darwin Kernel Version 19.6.0: macholo");
  if (!strcmp(name, "kern.maxfilesperproc")) return sysctl_return_i32(oldp, oldlenp, 10240);
  if (!strcmp(name, "kern.safeboot")) return sysctl_return_i32(oldp, oldlenp, 0);

  fprintf(stderr, "[sysctl] unhandled: %s\n", name);
  errno = ENOENT;
  return -1;
}

// The legacy raw sysctl(2) MIB interface. Only the handful of MIBs anyone still asks
// for are recognised; everything else reports "no such node" like a real kernel.
#define $darwin$CTL_KERN 1
#define $darwin$CTL_HW 6
#define $darwin$KERN_PROC 14
#define $darwin$KERN_PROC_PID 1
#define $darwin$HW_NCPU 3
#define $darwin$HW_PAGESIZE 7
#define $darwin$HW_MEMSIZE 24
#define $darwin$KERN_MAXFILESPERPROC 21
#define $darwin$KERN_OSVERSION 65

int $darwin$sysctl(int *name, unsigned namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen) {
  if (!name || namelen < 2) {
    errno = EINVAL;
    return -1;
  }
  if (name[0] == $darwin$CTL_HW && name[1] == $darwin$HW_NCPU)
    return $darwin$sysctlbyname("hw.ncpu", oldp, oldlenp, newp, newlen);
  if (name[0] == $darwin$CTL_HW && name[1] == $darwin$HW_MEMSIZE)
    return $darwin$sysctlbyname("hw.memsize", oldp, oldlenp, newp, newlen);
  if (name[0] == $darwin$CTL_HW && name[1] == $darwin$HW_PAGESIZE)
    return $darwin$sysctlbyname("hw.pagesize", oldp, oldlenp, newp, newlen);
  if (name[0] == $darwin$CTL_KERN && name[1] == $darwin$KERN_MAXFILESPERPROC)
    return $darwin$sysctlbyname("kern.maxfilesperproc", oldp, oldlenp, newp, newlen);
  if (name[0] == $darwin$CTL_KERN && name[1] == $darwin$KERN_OSVERSION)
    return $darwin$sysctlbyname("kern.osversion", oldp, oldlenp, newp, newlen);
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

void $darwin$uuid_generate(uint32_t out[2]) {
  // NOTE: don't really care in particular ?
  static uint64_t ticking = 0;
  out[0] = ++ticking;
  out[1] = random();
  out[2] = random();
  out[3] = ++ticking;
}

void $darwin$uuid_clear(uint8_t uuid[16]) {
  if (uuid) memset(uuid, 0, 16);
}

int $darwin$uname(struct $darwin$utsname *buf) {
  if (!buf) {
    errno = EFAULT;
    return -1;
  }
  memset(buf, 0, sizeof(struct $darwin$utsname));
  strcpy(buf->sysname, "Darwin");
  strcpy(buf->nodename, "macholo");
  strcpy(buf->release, "19.6.0");
  strcpy(buf->version, "Darwin Kernel Version 19.6.0: macholo");
  strcpy(buf->machine, "x86_64");
  return 0;
}

void $darwin$__cxa_throw() { panic("throwing dont work"); }

// Track 3 (docs/surface-audit/VERDICTS.md): the whole _SC_* namespace is
// renumbered between Darwin and glibc (Darwin _SC_NPROCESSORS_ONLN=58 is an
// unrelated Linux limit). Map the names, don't guess numbers.
long $darwin$sysconf(int name) {
  switch (name) {
    case 1: return sysconf(_SC_ARG_MAX);
    case 2: return sysconf(_SC_CHILD_MAX);
    case 3: return sysconf(_SC_CLK_TCK);
    case 4: return sysconf(_SC_NGROUPS_MAX);
    case 5: return sysconf(_SC_OPEN_MAX);
    case 8: return sysconf(_SC_VERSION);
    case 29: return sysconf(_SC_PAGESIZE);
    case 57: return sysconf(_SC_NPROCESSORS_CONF);
    case 58: return sysconf(_SC_NPROCESSORS_ONLN);
    case 200: return sysconf(_SC_PHYS_PAGES);
    default:
      fprintf(stderr, "[sysconf] unhandled darwin name: %d\n", name);
      *__errno_location() = EINVAL;
      return -1;
  }
}

// RLIMIT_* renumbered too: Darwin NOFILE=8 is Linux MEMLOCK, so raising the fd
// limit was silently locking memory instead. RLIM_INFINITY is 0x7fffffffffffffff
// on Darwin vs ~0UL on Linux; translate both directions.
#define DARWIN_RLIM_INFINITY 0x7fffffffffffffffull

static int rlimit_resource_to_gnu(int r) {
  switch (r) {
    case 0: return RLIMIT_CPU;
    case 1: return RLIMIT_FSIZE;
    case 2: return RLIMIT_DATA;
    case 3: return RLIMIT_STACK;
    case 4: return RLIMIT_CORE;
    case 5: return RLIMIT_AS;  // Darwin RLIMIT_AS == RLIMIT_RSS == 5
    case 6: return RLIMIT_MEMLOCK;
    case 7: return RLIMIT_NPROC;
    case 8: return RLIMIT_NOFILE;
    default: return -1;
  }
}

int $darwin$getrlimit(int resource, struct rlimit *rlp) {
  int l = rlimit_resource_to_gnu(resource);
  if (l < 0) {
    *__errno_location() = EINVAL;
    return -1;
  }
  int r = getrlimit(l, rlp);
  if (r == 0) {
    if (rlp->rlim_cur == RLIM_INFINITY || rlp->rlim_cur > DARWIN_RLIM_INFINITY) rlp->rlim_cur = DARWIN_RLIM_INFINITY;
    if (rlp->rlim_max == RLIM_INFINITY || rlp->rlim_max > DARWIN_RLIM_INFINITY) rlp->rlim_max = DARWIN_RLIM_INFINITY;
  }
  return r;
}

int $darwin$setrlimit(int resource, const struct rlimit *rlp) {
  int l = rlimit_resource_to_gnu(resource);
  if (l < 0) {
    *__errno_location() = EINVAL;
    return -1;
  }
  struct rlimit lim = *rlp;
  if (lim.rlim_cur == DARWIN_RLIM_INFINITY) lim.rlim_cur = RLIM_INFINITY;
  if (lim.rlim_max == DARWIN_RLIM_INFINITY) lim.rlim_max = RLIM_INFINITY;
  return setrlimit(l, &lim);
}

// struct rusage field offsets agree (the timeval tv_usec width difference hides
// in padding), but ru_maxrss is bytes on Darwin vs KiB on Linux.
int $darwin$getrusage(int who, struct rusage *ru) {
  int r = getrusage(who, ru);  // RUSAGE_SELF/CHILDREN agree
  if (r == 0) ru->ru_maxrss *= 1024;
  return r;
}
