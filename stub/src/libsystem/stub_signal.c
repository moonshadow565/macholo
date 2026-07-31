#include <common.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include "translate.h"

// Signal numbering (docs/surface-audit/VERDICTS.md): 1..6, 8, 9, 11, 13..15,
// 21, 22, 24..28 agree; the rest is permuted. Pairs are (darwin, linux);
// identity for anything not listed. Darwin SIGEMT(7) has no Linux number and
// SIGINFO(29) maps onto the otherwise-unused SIGPWR slot to stay injective.
static const struct { short d, l; } SIGPAIRS[] = {
    {10, 7},   // SIGBUS
    {12, 31},  // SIGSYS
    {16, 23},  // SIGURG
    {17, 19},  // SIGSTOP
    {18, 20},  // SIGTSTP
    {19, 18},  // SIGCONT
    {20, 17},  // SIGCHLD
    {23, 29},  // SIGIO
    {29, 30},  // SIGINFO -> SIGPWR (stand-in)
    {30, 10},  // SIGUSR1
    {31, 12},  // SIGUSR2
};

int macholo_signo_to_darwin(int s) {
  for (unsigned i = 0; i < sizeof(SIGPAIRS) / sizeof(*SIGPAIRS); i++)
    if (SIGPAIRS[i].l == s) return SIGPAIRS[i].d;
  return s;
}

int macholo_signo_to_linux(int s) {
  for (unsigned i = 0; i < sizeof(SIGPAIRS) / sizeof(*SIGPAIRS); i++)
    if (SIGPAIRS[i].d == s) return SIGPAIRS[i].l;
  return s;
}

int $darwin$kill(pid_t pid, int dsig) {
  return kill(pid, dsig ? macholo_signo_to_linux(dsig) : 0);
}

char *$darwin$strsignal(int dsig) {
  return strsignal(macholo_signo_to_linux(dsig));
}

// signal(): the host delivers the Linux number, the guest handler compares
// against Darwin constants -- trampoline translates. Guest handlers are kept per
// Linux signo; SIG_DFL/SIG_IGN/SIG_ERR values agree on both sides.
typedef void (*sig_handler_t)(int);
// Shared with $darwin$sigaction (stub_sys.c) via translate.h -- hidden, not exported.
sig_handler_t macholo_guest_handlers[64];

void macholo_sig_trampoline(int lsig) {
  sig_handler_t h = (lsig >= 0 && lsig < 64) ? macholo_guest_handlers[lsig] : NULL;
  if (h) h(macholo_signo_to_darwin(lsig));
}

// SA_* flag bits are permuted wholesale (Darwin SA_RESTART=0x2 vs Linux 0x10000000).
static const struct { unsigned d, l; } SAFLAGS[] = {
    {0x1, 0x8000000},   // SA_ONSTACK
    {0x2, 0x10000000},  // SA_RESTART
    {0x4, 0x80000000},  // SA_RESETHAND
    {0x8, 0x1},         // SA_NOCLDSTOP
    {0x10, 0x40000000}, // SA_NODEFER
    {0x20, 0x2},        // SA_NOCLDWAIT
};

unsigned macholo_saflags_to_linux(unsigned f) {
  unsigned l = 0;
  for (unsigned i = 0; i < sizeof(SAFLAGS) / sizeof(*SAFLAGS); i++)
    if (f & SAFLAGS[i].d) l |= SAFLAGS[i].l;
  return l;
}

unsigned macholo_saflags_to_darwin(unsigned f) {
  unsigned d = 0;
  for (unsigned i = 0; i < sizeof(SAFLAGS) / sizeof(*SAFLAGS); i++)
    if (f & SAFLAGS[i].l) d |= SAFLAGS[i].d;
  return d;
}

sig_handler_t $darwin$signal(int dsig, sig_handler_t handler) {
  int lsig = macholo_signo_to_linux(dsig);
  if (lsig <= 0 || lsig >= 64) {
    *__errno_location() = EINVAL;
    return SIG_ERR;
  }
  sig_handler_t prev_guest = macholo_guest_handlers[lsig];
  sig_handler_t installed;
  if (handler == SIG_DFL || handler == SIG_IGN) {
    macholo_guest_handlers[lsig] = NULL;
    installed = signal(lsig, handler);
  } else {
    macholo_guest_handlers[lsig] = handler;
    installed = signal(lsig, macholo_sig_trampoline);
  }
  if (installed == SIG_ERR) return SIG_ERR;
  // Previous disposition: a guest handler if one was installed, else whatever
  // the host reports (SIG_DFL/SIG_IGN, which need no translation).
  return prev_guest ? prev_guest : (installed == macholo_sig_trampoline ? SIG_DFL : installed);
}

// waitpid: option flags WNOHANG/WUNTRACED and the status bit layout agree; only
// the signal number embedded by WTERMSIG/WSTOPSIG is Linux-numbered.
pid_t $darwin$waitpid(pid_t pid, int *status, int options) {
  pid_t r = waitpid(pid, status, options);
  if (r > 0 && status) {
    int st = *status;
    if ((st & 0x7f) == 0x7f)  // stopped: signo in the high byte
      *status = (st & 0xff) | (macholo_signo_to_darwin((st >> 8) & 0xff) << 8);
    else if ((st & 0x7f) != 0)  // terminated by signal: signo in the low bits
      *status = (st & ~0x7f) | (macholo_signo_to_darwin(st & 0x7f) & 0x7f);
  }
  return r;
}
