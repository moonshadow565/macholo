# libsystem fall-through verdicts — Track 3 judgment pass

Input: [`libsystem.tsv`](libsystem.tsv) — every symbol the 16.3/16.15 game binaries import from
`libSystem`/`libresolv`, with how macholo's resolver satisfies it today. This document judges the
**214 rows the resolver currently answers by accident**: 209 `resolution=glibc` (resolver lane 4,
`dlsym(nullptr, name)`, zero ABI vetting) plus the 5 `resolution=UNRESOLVED`.

Output: [`libsystem-verdicts.tsv`](libsystem-verdicts.tsv) — `symbol, apple_sublib, verdict, reason, note`.

Verdicts:

- **FORWARD** — prototype, semantics, constants, struct layouts and error convention are identical on
  Darwin-x86_64 and glibc-x86_64. Safe to bind the same-named host symbol. These rows generate the
  forward allowlist (see *Resolver change spec*).
- **SHIM** — needs a `$darwin$`-prefixed translation shim in `stub/src/libsystem/`.
- **INVESTIGATE** — safety could not be established with high confidence; each row carries a `SETTLE BY:`
  in its note. Treat as SHIM until settled (i.e. **do not** put these in the allowlist).

Ground truths used, besides Darwin/xnu + glibc header knowledge:

1. The **108 existing `$darwin$` shims** (`nm -D build/stub/libstub.so`) mark the families the project
   already proved divergent. A glibc fall-through sibling inside such a family is a red flag and is
   called out per row (`socket` next to shimmed `bind`/`connect`; `open`/`fcntl` next to shimmed
   `stat$INODE64`/`mmap`; `signal`/`kill` next to shimmed `sigaction`; `strtod_l` next to shimmed
   `vsnprintf_l`; `posix_spawnattr_*` next to shimmed `posix_spawn_file_actions_*`).
2. The stub sources themselves, which already encode Darwin constants and struct layouts
   (`stub_socket.c` AF table, `stub_fs.c` `struct $darwin$stat64`/`statfs64`/`dirent64`,
   `stub_str.c` `$darwin$mmap` MAP_\* translation, `stub_sys.c` `struct $darwin$sigaction`).

---

## 1. Counts

### verdict x reason

| verdict | reason | count |
|---|---|---|
| FORWARD | - | 173 |
| INVESTIGATE | semantics | 2 |
| INVESTIGATE | struct | 1 |
| SHIM | flags | 17 |
| SHIM | struct | 12 |
| SHIM | signature | 5 |
| SHIM | semantics | 3 |
| SHIM | errno | 1 |
| **total** | | **214** |

### verdict totals

| verdict | count |
|---|---|
| FORWARD | 173 |
| SHIM | 38 |
| INVESTIGATE | 3 |

### apple_sublib x verdict

| apple_sublib | FORWARD | SHIM | INVESTIGATE |
|---|---|---|---|
| libdispatch | 4 | 0 | 0 |
| libsystem_c | 95 | 18 | 3 |
| libsystem_kernel | 28 | 16 | 0 |
| libsystem_m | 41 | 0 | 0 |
| libsystem_malloc | 4 | 0 | 0 |
| libsystem_platform | 0 | 4 | 0 |
| libunwind | 1 | 0 | 0 |

`libsystem_m` is 41/41 FORWARD: every demanded math symbol takes and returns `double`/`float` in SSE
registers, or writes through a pointer out-param (`frexp`, `ldexp`, `modf`, `modff`). **There is no
struct/complex return and no `long double` entry point left in the fall-through set** — the one
struct-returning math symbol the game imports, `__sincosf_stret`, is already shimmed, and `__exp10`/
`__exp10f` are already shimmed too. The `long double` arm64 caveat therefore lands on the
printf/scanf family instead, not on libm (see §5).

---

## 2. SHIM and INVESTIGATE, grouped by target module file

Grouping follows Apple's own `libsystem_*` sublibrary boundaries as track 1 laid them out; two new
files are needed (`stub_locale.c`, `stub_setjmp.c`). Every symbol below is bound to a same-named
glibc symbol **today**.

### `stub/src/libsystem/stub_fs.c` (libsystem_kernel — paths, fds, xattr) — 5

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_fcntl` | libsystem_kernel | SHIM | flags | F_GETLK/F_SETLK/F_SETLKW are 7/8/9 on Darwin vs 5/6/7 on Linux, F_GETOWN/F_SETOWN 5/6 vs 9/8, F_DUPFD_CLOEXEC 67 vs 1030; struct flock field order differs; F_GETPATH/F_FULLFSYNC/F_NOCACHE/F_RDADVISE/F_PREALLOCATE have no Linux equivalent |
| `_getxattr` | libsystem_kernel | SHIM | signature | Darwin: ssize_t getxattr(path,name,value,size,uint32_t position,int options) -- 6 args. Linux: 4 args. The extra Darwin args land in registers glibc never reads, and XATTR_NOFOLLOW is lost |
| `_open` | libsystem_kernel | SHIM | flags | HEADLINE. Darwin O_CREAT=0x0200 O_TRUNC=0x0400 O_EXCL=0x0800 O_NONBLOCK=0x0004 O_APPEND=0x0008 O_CLOEXEC=0x1000000; Linux O_CREAT=0x40 O_TRUNC=0x200 O_EXCL=0x80 O_NONBLOCK=0x800 O_APPEND=0x400 O_CLOEXEC=0x80000. Darwin O_RDWR\|O_CREAT\|O_TRUNC lands on Linux as O_RDWR\|O_TRUNC\|O_APPEND with NO O_CREAT |
| `_removexattr` | libsystem_kernel | SHIM | signature | Darwin: removexattr(path,name,int options) -- 3 args; Linux takes 2. options (XATTR_NOFOLLOW) lost |
| `_setxattr` | libsystem_kernel | SHIM | signature | Darwin: setxattr(path,name,value,size,uint32_t position,int options). Linux: setxattr(path,name,value,size,int flags) -- Darwin's arg5 'position' is read by glibc as 'flags', so a position of 0 means flags=0 by luck and anything else is garbage; arg6 options is dropped |

### `stub/src/libsystem/stub_io.c` (libsystem_c/kernel — stdio, poll, termios) — 4

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_poll` | libsystem_kernel | SHIM | signature | nfds_t is unsigned int on Darwin but unsigned long on Linux: the guest passes 32 bits in %esi and the upper half of %rsi is undefined per the SysV ABI, which glibc's poll() reads. One-line truncating wrapper. POLLIN/OUT/ERR/HUP/NVAL values do match |
| `_setbuf` | libsystem_c | SHIM | flags | BUFSIZ is 1024 on Darwin and 8192 in glibc, and setbuf() means setvbuf(fp,buf,_IOFBF,BUFSIZ). glibc overruns the guest's BUFSIZ-sized buffer by 7 KiB. setvbuf (explicit size) is fine |
| `_tcgetattr` | libsystem_c | SHIM | struct | struct termios is a different shape: Darwin tcflag_t/speed_t are unsigned long (64-bit) with NCCS=20 and no c_line; glibc uses unsigned int with c_line and NCCS=32. VMIN/VTIME live at index 16/17 vs 6/5, and ICANON is 0x100 vs 0x2 |
| `_tcsetattr` | libsystem_c | SHIM | struct | same struct termios divergence as tcgetattr; TCSANOW/TCSADRAIN/TCSAFLUSH 0/1/2 do match |

### `stub/src/libsystem/stub_socket.c` (libsystem_kernel + libsystem_c BSD sockets) — 4

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_recv` | libsystem_c | SHIM | flags | MSG_* diverge above bit 3: Darwin MSG_WAITALL=0x40 is Linux MSG_DONTWAIT, Darwin MSG_DONTWAIT=0x80 is Linux MSG_EOR, Darwin MSG_TRUNC=0x10 is Linux MSG_PROBE. recvfrom/sendto are already shimmed |
| `_send` | libsystem_c | SHIM | flags | same MSG_* divergence as recv; also Darwin MSG_NOSIGNAL=0x80000 has no Linux counterpart (Linux MSG_NOSIGNAL=0x4000) |
| `_socket` | libsystem_kernel | SHIM | flags | AF_INET6 is 30 on Darwin vs 10 on Linux (30 == AF_TIPC), AF_ROUTE 17 vs AF_PACKET, AF_SNA 11 vs 22. Sibling red flag: bind/connect/accept/getsockname/getsockopt already have $darwin$ shims that map sa_family, but socket() itself falls through unmapped |
| `_socketpair` | libsystem_kernel | SHIM | flags | same AF_* divergence as socket(); reuse the stub_socket.c sa_family_to_gnu table |

### `stub/src/libsystem/stub_process.c` (libsystem_kernel + libsystem_c — spawn and signals) — 9

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_kill` | libsystem_kernel | SHIM | flags | signal numbers diverge from 10 up: Darwin SIGBUS=10 is Linux SIGUSR1, Darwin SIGSYS=12 is Linux SIGUSR2, Darwin SIGCHLD=20 is Linux SIGTSTP, Darwin SIGUSR1=30 is Linux SIGPWR |
| `_posix_spawn` | libsystem_kernel | SHIM | struct | CRITICAL. Darwin posix_spawnattr_t / posix_spawn_file_actions_t are 8-byte opaque pointers; glibc's are 336- and 80-byte inline structs the caller must allocate. POSIX_SPAWN_* also diverge above 0x08. The file_actions half is already shimmed to panic (stub_process.c) |
| `_posix_spawnattr_destroy` | libsystem_kernel | SHIM | struct | same 8-vs-336-byte posix_spawnattr_t divergence as posix_spawnattr_init |
| `_posix_spawnattr_init` | libsystem_kernel | SHIM | struct | CRITICAL / memory corruption. The guest allocates sizeof(posix_spawnattr_t) == 8 bytes (Darwin's opaque pointer) and glibc's posix_spawnattr_init memsets 336 bytes over it -- a 328-byte stack smash on the very first call |
| `_posix_spawnattr_setflags` | libsystem_kernel | SHIM | struct | same 8-vs-336-byte posix_spawnattr_t divergence, plus POSIX_SPAWN_* values diverge above 0x08 (Darwin SETEXEC=0x40 / START_SUSPENDED=0x80 vs glibc USEVFORK=0x40 / SETSID=0x80) |
| `_posix_spawnp` | libsystem_c | SHIM | struct | posix_spawnattr_t and posix_spawn_file_actions_t are 8-byte opaque pointers on Darwin but 336- and 80-byte inline structs in glibc, and POSIX_SPAWN_* diverge above 0x08 (Darwin SETEXEC=0x40, START_SUSPENDED=0x80). Also sigset_t is 4 bytes vs 128 |
| `_signal` | libsystem_c | SHIM | flags | same signo divergence as kill; SIG_DFL/SIG_IGN/SIG_ERR do match. Sibling red flag: $darwin$sigaction already exists (stub_sys.c) but only special-cases SIGPIPE, which is 13 on both |
| `_strsignal` | libsystem_c | SHIM | flags | signo divergence: strsignal(Darwin SIGCHLD=20) returns glibc's "Stopped" |
| `_waitpid` | libsystem_c | SHIM | flags | LOW PRIORITY. WNOHANG/WUNTRACED and the WIFEXITED/WEXITSTATUS bit encoding are identical; only the signal number inside WTERMSIG/WSTOPSIG is Linux-numbered. Fold into the kill/signal signo table |

### `stub/src/libsystem/stub_sys.c` (libsystem_kernel + libsystem_c — limits, clocks, sysconf) — 6

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_strptime` | libsystem_c | INVESTIGATE | semantics | signature and return convention match, but Darwin's strptime is FreeBSD-derived and glibc's differs on %Z (glibc only accepts known zone names), on %p outside the C locale, and on whether unmatched struct tm fields are left untouched. SETTLE BY: running the format strings the 16.15 binary actually passes (grep the __TEXT,__cstring refs at the call sites) through both libcs |
| `_clock_gettime` | libsystem_c | SHIM | flags | HIGH IMPACT. Darwin CLOCK_MONOTONIC=6 lands on Linux CLOCK_MONOTONIC_COARSE=6, so the game's frame clock silently degrades to ~4 ms granularity instead of failing. Darwin CLOCK_THREAD_CPUTIME_ID=16 and CLOCK_PROCESS_CPUTIME_ID=12 are invalid on Linux; CLOCK_REALTIME=0 and CLOCK_MONOTONIC_RAW=4 happen to match. struct timespec is identical |
| `_getrlimit` | libsystem_kernel | SHIM | flags | RLIMIT_NOFILE is 8 on Darwin but 7 on Linux (8 == RLIMIT_MEMLOCK); NPROC 7 vs 6; AS 5 vs 9. RLIM_INFINITY is 0x7fffffffffffffff on Darwin vs ~0UL on Linux |
| `_getrusage` | libsystem_kernel | SHIM | struct | ru_utime/ru_stime are struct timeval, whose tv_usec is int32 on Darwin vs long on glibc; ru_maxrss is documented in BYTES on Darwin but KiB on Linux. RUSAGE_SELF/CHILDREN do match |
| `_setrlimit` | libsystem_kernel | SHIM | flags | same RLIMIT_* / RLIM_INFINITY divergence as getrlimit; raising the fd limit silently sets RLIMIT_MEMLOCK instead |
| `_sysconf` | libsystem_c | SHIM | flags | the entire _SC_* namespace is renumbered: Darwin _SC_PAGESIZE=29 / _SC_NPROCESSORS_ONLN=58 / _SC_PHYS_PAGES=200 vs Linux 30 / 84 / 85, and every name below that is off by one. A core-count query returns an unrelated limit or -1 |

### `stub/src/libsystem/stub_str.c` (libsystem_c — string/path/errno text) — 4

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_basename` | libsystem_c | SHIM | semantics | dlsym picks glibc's GNU basename (from <string.h>), which never strips trailing slashes: basename("/a/b/") is "" on glibc but "b" on Darwin, and "" maps to "." on Darwin. The POSIX one in glibc is __xpg_basename |
| `_dirname` | libsystem_c | SHIM | semantics | glibc's dirname MODIFIES its argument in place; Darwin's copies into a per-thread buffer and leaves the input alone. A guest passing a string literal or a shared buffer gets it clobbered |
| `_strerror` | libsystem_c | SHIM | errno | errno numbering diverges from 11 up (Darwin EDEADLK=11 vs Linux EAGAIN=11) and completely from 35 up (Darwin EAGAIN=35 vs Linux EDEADLK=35). Guest call sites pass literal Darwin E* constants. See the cross-cutting errno section in VERDICTS.md |
| `_strerror_r` | libsystem_c | SHIM | signature | glibc's exported strerror_r is the GNU variant returning char* and possibly not touching the buffer; Darwin's is XSI (int return, always fills the buffer). The guest reads a truncated pointer as the int result and an untouched buffer. glibc's XSI variant is __xpg_strerror_r |

### `stub/src/libsystem/stub_locale.c` — **NEW FILE** (libsystem_c — locale) — 4

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_localeconv` | libsystem_c | INVESTIGATE | struct | the 10 leading char* fields definitely agree, but Darwin appears to order the trailing chars int_p_cs_precedes, int_n_cs_precedes, int_p_sep_by_space, int_n_sep_by_space, ... while glibc orders them int_p_cs_precedes, int_p_sep_by_space, int_n_cs_precedes, ... SETTLE BY: diffing a real macOS <locale.h> struct lconv. Only decimal_point/thousands_sep matter to a game, so the hazard is small either way |
| `_newlocale` | libsystem_c | SHIM | flags | LC_*_MASK is 1<<category, so the permuted category numbers permute the masks too (Darwin LC_ALL_MASK=0x3f vs glibc's 0x1fbf). locale_t itself is an opaque pointer on both |
| `_setlocale` | libsystem_c | SHIM | flags | LC_* categories are permuted: Darwin LC_ALL=0 LC_COLLATE=1 LC_CTYPE=2 LC_MONETARY=3 LC_NUMERIC=4 LC_TIME=5 LC_MESSAGES=6; glibc LC_CTYPE=0 LC_NUMERIC=1 LC_TIME=2 LC_COLLATE=3 LC_MONETARY=4 LC_MESSAGES=5 LC_ALL=6. setlocale(LC_ALL,"") therefore only sets LC_CTYPE |
| `_strtod_l` | libsystem_c | SHIM | semantics | Darwin's *_l functions normalise loc==NULL to the global locale (NORMALIZE_LOCALE); glibc's dereference it and crash. Sibling evidence: $darwin$vsnprintf_l already exists |

### `stub/src/libsystem/stub_setjmp.c` — **NEW FILE** (libsystem_platform) — 4

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `__longjmp` | libsystem_platform | SHIM | struct | CRITICAL / memory corruption. Darwin x86_64 jmp_buf is int[_JBLEN] == int[37] == 148 bytes; glibc's struct __jmp_buf_tag is 200 bytes (64-byte __jmp_buf + int + 128-byte sigset). glibc's setjmp writes 52 bytes past the end of the guest's buffer. Mask semantics do agree (setjmp saves it, _setjmp does not). arm64 caveat: the sizes differ again there (Darwin 384 bytes vs glibc 320), so the shim must be per-arch -- _longjmp must read back whatever the _setjmp shim wrote |
| `__setjmp` | libsystem_platform | SHIM | struct | CRITICAL / memory corruption. Darwin x86_64 jmp_buf is int[_JBLEN] == int[37] == 148 bytes; glibc's struct __jmp_buf_tag is 200 bytes (64-byte __jmp_buf + int + 128-byte sigset). glibc's setjmp writes 52 bytes past the end of the guest's buffer. Mask semantics do agree (setjmp saves it, _setjmp does not). arm64 caveat: the sizes differ again there (Darwin 384 bytes vs glibc 320), so the shim must be per-arch |
| `_longjmp` | libsystem_platform | SHIM | struct | CRITICAL / memory corruption. Darwin x86_64 jmp_buf is int[_JBLEN] == int[37] == 148 bytes; glibc's struct __jmp_buf_tag is 200 bytes (64-byte __jmp_buf + int + 128-byte sigset). glibc's setjmp writes 52 bytes past the end of the guest's buffer. Mask semantics do agree (setjmp saves it, _setjmp does not). arm64 caveat: the sizes differ again there (Darwin 384 bytes vs glibc 320), so the shim must be per-arch -- longjmp must read back whatever the setjmp shim wrote |
| `_setjmp` | libsystem_platform | SHIM | struct | CRITICAL / memory corruption. Darwin x86_64 jmp_buf is int[_JBLEN] == int[37] == 148 bytes; glibc's struct __jmp_buf_tag is 200 bytes (64-byte __jmp_buf + int + 128-byte sigset). glibc's setjmp writes 52 bytes past the end of the guest's buffer. Mask semantics do agree (setjmp saves it, _setjmp does not). arm64 caveat: the sizes differ again there (Darwin 384 bytes vs glibc 320), so the shim must be per-arch |

### `stub/src/libsystem/stub_compat.c` (libsystem_c — misc Darwin-isms) — 1

| symbol | sublib | verdict | reason | why |
|---|---|---|---|---|
| `_backtrace` | libsystem_c | INVESTIGATE | semantics | prototype is identical, but glibc's backtrace() runs _Unwind_Backtrace, which resolves FDEs via dl_iterate_phdr and therefore sees no ELF object for a Mach-O guest frame; libgcc returns _URC_FATAL_PHASE1_ERROR rather than a clean end-of-stack in that case. SETTLE BY: calling it from guest code in the 16.15 sample and checking it returns instead of faulting. Sibling evidence: backtrace_from_fp is already stubbed to return 0, so stubbing this to 0 is the cheap safe answer |

---

## 3. The five "UNRESOLVED" symbols — all five already resolve; the audit's host-lib list was incomplete

`tools/surface-audit.sh` models resolver lane 4 as "is the name in `libc.so.6` or `libm.so.6`", plus a
fixed host list of `libGL.so.1`, `libasound.so.2`, `libSDL2-2.0.so.0`. The real lane 4 is
`dlsym(nullptr, name)` = `RTLD_DEFAULT`, which searches **the executable and every globally-loaded
dependency** — for macholo that is ~40 libraries including `libgcc_s.so.1` and `libdispatch.so`
(both pulled in transitively through `libstub.so` -> libc++/GNUstep). All five "UNRESOLVED" rows are
artifacts of that gap, verified on this host by `dlsym(RTLD_DEFAULT, …)` + `dladdr` from a binary
linked exactly like macholo:

```
_Unwind_Resume               0x…  /lib64/libgcc_s.so.1
dispatch_once_f              0x…  /lib64/libdispatch.so
dispatch_semaphore_create    0x…  /lib64/libdispatch.so
dispatch_semaphore_signal    0x…  /lib64/libdispatch.so
dispatch_semaphore_wait      0x…  /lib64/libdispatch.so
```

### `dispatch_once_f`, `dispatch_semaphore_{create,signal,wait}` — verdict FORWARD (override-forward)

`/usr/lib64/libdispatch.so` is `libdispatch-6.0.2` = **swift-corelibs-libdispatch**, i.e. Apple's own
libdispatch source built for Linux. The types are ABI-identical by construction (`dispatch_once_t` is
`long`, `dispatch_semaphore_t` is an opaque pointer, `dispatch_time_t` is `uint64_t`). Behaviour was
verified rather than assumed:

- `dispatch_once_f(&pred, ctx, fn)` runs the callout exactly once and leaves `pred == 0xffffffffffffffff`.
  That sentinel is load-bearing: the guest never calls `dispatch_once` itself, it calls the **inlined**
  fast path from Apple's `<dispatch/once.h>`, which tests `*pred != ~0l` before falling through to
  `dispatch_once_f`. The host value matches Darwin's `DLOCK_ONCE_DONE`, so the fast path arms correctly.
  A hand-rolled pthread_once-based shim would **break** this unless it wrote the same sentinel.
- `dispatch_semaphore_create(0)` then `wait(DISPATCH_TIME_NOW == 0)` returns non-zero (timeout),
  `signal()` returns 0, and `wait(DISPATCH_TIME_FOREVER == ~0ull)` returns 0. Matches Darwin.

So: no shim, no pthread reimplementation. Add all four to the forward allowlist and fix
`tools/surface-audit.sh` to model lane 4 as the actual global scope (see §6).

Residual note: the guest imports no `dispatch_release`/`dispatch_retain`, so created semaphores leak.
That is a guest-side lifetime question, not an ABI one.

### `_Unwind_Resume` — verdict FORWARD (override-forward), with a live caveat

`libgcc_s.so.1` is a transitive `DT_NEEDED` of `macholo` (the loader's own C++ code has an undefined
`_Unwind_Resume@GCC_3.0`), so lane 4 binds it already. The prototype is the shared Level-1 Itanium
unwind ABI — `void _Unwind_Resume(struct _Unwind_Exception *)` — identical between libgcc and Apple's
libunwind. It is also **unreachable today**: `_Unwind_Resume` is only entered from a landing pad
during a live unwind, and `$darwin$__cxa_throw` (`stub_sys.c`) panics before any unwind can start.

Caveat to record for whoever enables guest C++/ObjC exceptions: libgcc's unwinder locates FDEs through
`dl_iterate_phdr`, which enumerates ELF objects only. Mach-O `__TEXT,__eh_frame` / compact-unwind data
in a guest image is invisible to it, so a real unwind through guest frames aborts rather than resumes.
At that point this row becomes SHIM/semantics and the honest first step is a `$darwin$_Unwind_Resume`
that panics with a clear message, matching `$darwin$__cxa_throw`.

Implementation sketch if you ever do need to own it: a `$darwin$_Unwind_Resume` that
`panic("guest exception unwind is not supported")`; a real implementation means teaching the
unwinder about guest images (register the Mach-O `__eh_frame` via `__register_frame`, or ship
Apple's libunwind and route the whole `_Unwind_*` family to it), which is its own track.

---

## 4. Cross-cutting: the errno numbering policy

Not a per-symbol verdict, but it sits underneath the whole `resolution=glibc` set and should be
decided before the `strerror` shim is written.

`$darwin$__error()` (`stub_kernel.c`) returns glibc's `__errno_location()` verbatim, so the guest reads
**glibc-numbered** errno values. Darwin and Linux agree on 1..10, then diverge: Darwin `EDEADLK=11`
vs Linux `EAGAIN=11`, and from 35 up nothing lines up (Darwin `EAGAIN=35` vs Linux `EDEADLK=35`,
Darwin `ENOTSOCK=38` vs Linux `ENOSYS=38`, …). Guest code compiled against Darwin headers compares
`errno` against *Darwin* constants, so every `if (errno == EAGAIN)` in the guest is already wrong.

Two coherent policies; pick one and make the shims follow it:

- **A (status quo, cheapest):** keep exposing glibc numbering. Then `strerror` is self-consistent for
  values that came out of `errno`, and only guest call sites that pass a *literal* `E*` misbehave.
  Ship the `strerror` shim as a Darwin-number -> string table anyway, because literals are exactly what
  gets passed to `strerror`.
- **B (correct, more work):** make `$darwin$__error()` return a thread-local *Darwin*-numbered errno and
  translate at every syscall-wrapper shim boundary. That turns `errno` into a shimmed resource and adds
  a translation step to every `SHIM` row in §2 that can fail.

Until this is decided, `_strerror` / `_strerror_r` stay SHIM (`_strerror_r` needs a shim under either
policy — glibc's exported `strerror_r` is the GNU `char *`-returning variant, not Darwin's XSI `int` one).

**Decision (2026-07-31): Policy A.** Both reference games are regression-verified under glibc
numbering, and B breaks the `errno = 0; call(); if (errno)` idiom for FORWARD-lane calls unless
`__error()` grows refresh-on-read machinery. Ship the Darwin-table `strerror`/`strerror_r` shims
under A; revisit B as its own item if a guest is ever caught branching on a diverged `E*` literal
(the socket-`EAGAIN` family is the likely first offender — Darwin 35 vs Linux 11).

---

## 5. arm64 caveats parked for the aarch64/Android track

Carried in the `note` column of the affected rows so they survive to the port:

- **`long double`** — Darwin arm64 makes `long double` a 64-bit `double`; AAPCS64/glibc makes it
  128-bit binary128. Every `%Lf`/`%La` in the printf/scanf family (`_fprintf`, `_sprintf`, `_snprintf`,
  `_sscanf`, `___sprintf_chk` and the `v*` variants) is FORWARD on x86_64 **only**. Not a libm issue:
  the fall-through set contains no `long double` math entry points.
- **`va_list`** — x86_64 SysV `va_list` is identical on both platforms; on arm64 Darwin uses a plain
  `char *` while AAPCS64 uses a struct. `_vfprintf`, `_vsprintf`, `_vsnprintf`, `_vsscanf` must be
  re-judged there (and the existing `$darwin$vsnprintf_l` shim re-checked).
- **`jmp_buf`** — sizes diverge on arm64 too, differently (Darwin 384 bytes vs glibc 320), so the
  `stub_setjmp.c` shim has to be per-arch rather than a single fixed translation.
- **narrow argument extension** — x86_64 leaves 16->32-bit extension to the caller in practice, which
  is why `_chmod`/`_mkdir`/`_umask` (`mode_t` is `uint16_t` on Darwin) are FORWARD here. Apple's arm64
  ABI moves that duty to the callee, so those rows need re-checking on the port. The 32->64-bit case
  is already a defect on x86_64 — see `_poll`/`nfds_t`.
- **bionic-vs-glibc** — orthogonal to everything above and already on the REFACTOR.md checklist; the
  allowlist format in §6 is the natural place to hang a per-libc column when that starts.

---

## 6. Determinism watch list (FORWARD, but not bit-identical)

These are ABI-correct — they are in the allowlist and need no shim — but they do not reproduce Apple's
*values*. Listed because the project's stated end goal is a replay-playback client, where a one-ulp or
one-permutation difference inside game logic desyncs a replay:

- `_rand` / `_srand` / `_rand_r` — Darwin uses the 16807 minimal-standard LCG, glibc a TYPE_3 additive
  feedback generator. Same `RAND_MAX`, completely different stream from the same seed.
- `_qsort` — ordering of *equal* elements differs (Darwin introsort-family vs glibc mergesort).
- `_strcoll` — collation order differs under non-C locales.
- the 31 transcendental `libsystem_m` entries (`sin`, `cos`, `exp`, `pow`, `log`, …) — up to ~1 ulp
  apart from Apple's libm.

If a desync is ever traced to one of these, the fix is a `$darwin$` shim that reimplements Apple's
algorithm; nothing about the current binding is unsafe.

---

## 7. Resolver change spec

Concrete enough to implement mechanically; no design decisions left open.

### 7.1 Generated artifact

`tools/surface-audit.sh` gains a final step that regenerates a checked-in header from
`docs/surface-audit/libsystem-verdicts.tsv`, **FORWARD rows only**:

```sh
# ---------------------------------------------------- 5. forward allowlist ---
# Generated from the Track 3 verdicts table: lane 4 (dlsym(nullptr, name)) may
# only answer for symbols judged ABI-identical on Darwin-x86_64 vs glibc-x86_64.
verdicts="$out_dir/libsystem-verdicts.tsv"
inc="$root/src/forward_allowlist.inc"
{
  printf '// GENERATED by tools/surface-audit.sh from docs/surface-audit/libsystem-verdicts.tsv\n'
  printf '// Do not edit by hand: change the verdict in the TSV and re-run the audit.\n'
  awk -F'\t' 'NR > 1 && $3 == "FORWARD" {
        sym = $1
        sub(/^_/, "", sym)          # resolve_import() strips exactly one leading underscore
        print sym
      }' "$verdicts" | LC_ALL=C sort -u | sed -E 's/.*/FORWARD("&")/'
} > "$inc"
```

Notes for the implementer:

- Strip exactly one leading `_`, matching `lib::resolve_import()` in `src/lib.cpp`. `__setjmp` -> `_setjmp`,
  `___memcpy_chk` -> `__memcpy_chk`, `__Unwind_Resume` -> `_Unwind_Resume`.
- Sort + `-u` so the file is stable across runs and diffs cleanly.
- The allowlist is **name-only**. It does not record which host library provides the symbol, because
  lane 4 is `RTLD_DEFAULT` over the whole global scope (~40 libraries), not just libc/libm.
- SHIM and INVESTIGATE rows are deliberately absent: they must fail the allowlist until a `$darwin$`
  shim exists, at which point lane 1 answers first and lane 4 is never reached for them.

### 7.2 `src/lib.cpp` lane 4

Replace the trailing `return ::dlsym(nullptr, name.data());` in `lib::resolve()` with:

```cpp
  static std::string_view const FORWARD_ALLOWLIST[] = {
#define FORWARD(sym) sym,          // X-macro: the .inc lines are bare FORWARD("x") with no commas
#include "forward_allowlist.inc"
#undef FORWARD
  };

  auto const allowed = std::binary_search(std::begin(FORWARD_ALLOWLIST), std::end(FORWARD_ALLOWLIST), name);
  if (!allowed) {
    static bool const strict = [] {
      char const *e = ::getenv("MACHOLO_STRICT_RESOLVE");
      return e && e[0] == '1' && e[1] == '\0';
    }();
    if (strict) panic("resolve: %.*s is not on the Track 3 forward allowlist", (int)name.size(), name.data());
    warn_once(name);  // see 7.3
  }
  return ::dlsym(nullptr, name.data());
```

Rules:

- The allowlist gate sits **after** the `$darwin$` shim lane, the `OVERRIDE` table and the `BLOCKED`
  prefix list, i.e. it only ever sees names those three did not claim. Do not reorder the existing lanes.
- `std::binary_search` requires the `.inc` to be sorted; §7.1 guarantees that. Add a
  `static_assert(std::is_sorted(...))` in a constexpr context if you want it enforced at build time.
- **Default (`MACHOLO_STRICT_RESOLVE` unset or != `1`): warn once per symbol, then forward anyway.**
  Unknown-but-present symbols keep working, so this change cannot regress a currently-booting game.
- **`MACHOLO_STRICT_RESOLVE=1`: hard error** via the existing `panic()` in `src/common.hpp`. This is the
  mode CI and `tools/gate.sh` should run in.
- Behaviour when `dlsym` itself returns null is unchanged: `lib::resolve()` returns null and
  `resolve_import()`'s existing caller (`stub::dyld_stub_binder` / the eager bind loop) reports it.

**Supervisor correction (2026-07-31) — provenance-gate the check.** Lane 4 is how *every*
plain stub-lib export, GNUstep CF/NS symbol and libobjc symbol resolves too (that is the flat
namespace working as designed) — none of those are in the allowlist, so the gate as specified
above would warn on hundreds of legitimate symbols and panic in strict mode on the first
AppKit class ref. The allowlist requirement applies only to the audited-risk domain: after
`dlsym` succeeds, `dladdr` the result; enforce (warn / strict-panic) **only if the defining
object is glibc's `libc.so.6` or `libm.so.6`**. Resolutions provided by `libstub.so`, the
GNUstep/sysroot libs, `libobjc`, `libdispatch`, `libgcc_s`, SDL/GL/asound pass silently.
`dladdr` cost is paid once per symbol (bind results are cached upstream), and only on the
not-allowlisted path.

### 7.3 `warn_once`

A file-local helper in `src/lib.cpp`, defined **above** `lib::resolve()` (no forward declaration needed
then). It reuses the existing `warn_unresolved` knob from `src/common.hpp` so the whole category can be
muted from one place:

```cpp
static void warn_once(std::string_view name) {
  static std::mutex mutex;
  static std::set<std::string, std::less<>> seen;
  auto const lock = std::lock_guard{mutex};
  if (!seen.emplace(name).second) return;
  warn_unresolved("[resolve] %.*s: not audited, forwarding to the host global scope anyway "
                  "(set MACHOLO_STRICT_RESOLVE=1 to make this fatal)\n",
                  (int)name.size(), name.data());
}
```

`std::less<>` gives heterogeneous lookup so no `std::string` is materialised on the repeat path.
Binding happens on worker threads through `dyld_stub_binder`, hence the mutex.

### 7.4 `tools/surface-audit.sh` fixes that fall out of this pass

1. **Model lane 4 as the real global scope.** The current `glibc` bucket only greps `libc.so.6` +
   `libm.so.6`, and the `host` bucket is a hardcoded three-library list. That is what produced the five
   phantom `UNRESOLVED` rows in §3. Replace both with the actual `DT_NEEDED` closure of
   `build/macholo` (`ldd build/macholo`), and keep the providing library in the resolution string so
   `glibc:libc.so.6` / `glibc:libdispatch.so` stay distinguishable.
2. **Emit `src/forward_allowlist.inc`** per §7.1.
3. **Add a drift check**: fail the run if any symbol in `libsystem.tsv` with `resolution=glibc` has no
   row in `libsystem-verdicts.tsv`. New game-version demand then shows up as an audit failure rather
   than as a silent new fall-through.

### 7.5 Order of work

1. §7.1 + §7.2 + §7.3 — allowlist plumbing, warn-by-default. No behaviour change for a working game.
2. The four **CRITICAL** memory-corruption rows first: `_setjmp`/`_longjmp`/`__setjmp`/`__longjmp`
   (`stub_setjmp.c`) and `_posix_spawnattr_init` + siblings (`stub_process.c`).
3. `_open` and `_fcntl` (`stub_fs.c`) — the flag translation with the widest blast radius.
4. `_clock_gettime`, `_sysconf`, `_getrlimit`/`_setrlimit` (`stub_sys.c`) — silently-wrong values that
   degrade the game rather than crash it, so they are the ones no test will catch.
5. Everything else in §2, then flip CI to `MACHOLO_STRICT_RESOLVE=1`.
6. Settle the three INVESTIGATE rows and the §4 errno policy.

