#include <errno.h>
#include <error.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <common.h>

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

int $darwin$sigaction(int sig, const struct $darwin$sigaction *act, struct $darwin$sigaction *oact) {
  struct sigaction gnu_act;
  struct sigaction gnu_oact;

  if (sig == SIGPIPE) {
    // getting action
    if (act == NULL && oact != NULL) {
      int result = sigaction(sig, NULL, &gnu_oact);
      oact->sigaction_u = gnu_oact.sa_handler;
      oact->sa_flags = gnu_oact.sa_flags;
      return result;
    }

    // setting action
    if (act != NULL && oact == NULL) {
      // GNU has restorer as well, need to get it
      sigaction(sig, NULL, &gnu_act);
      gnu_act.sa_handler = act->sigaction_u;
      gnu_act.sa_flags = act->sa_flags;
      int result = sigaction(sig, &gnu_act, NULL);
      return result;
    }
  }

  fprintf(stderr, "ignoring sigaction: %d...\n", sig);
  return 0;
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

void $darwin$uuid_generate(uint32_t out[2]) {
  // NOTE: don't really care in particular ?
  static uint64_t ticking = 0;
  out[0] = ++ticking;
  out[1] = random();
  out[2] = random();
  out[3] = ++ticking;
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
