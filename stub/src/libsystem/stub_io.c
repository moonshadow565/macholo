#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>

#include <common.h>

int $darwin$select$1050(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, void *timeout) {
  // Darwin timeval is {int64 tv_sec; int32 tv_usec; <4 pad>} -- passing it through
  // lets glibc read the padding as the top half of its 64-bit tv_usec. Widen.
  struct { int64_t tv_sec; int32_t tv_usec; } *dtv = timeout;
  struct timeval tv;
  if (dtv) {
    tv.tv_sec = dtv->tv_sec;
    tv.tv_usec = dtv->tv_usec;
  }
  int r = select(nfds, readfds, writefds, exceptfds, dtv ? &tv : NULL);
  if (dtv) {  // Linux select updates the remaining time; reflect it back
    dtv->tv_sec = tv.tv_sec;
    dtv->tv_usec = (int32_t)tv.tv_usec;
  }
  return r;
}

int $darwin$connectx(int, const void *, void *, unsigned int, const void *, unsigned int, size_t *, void *) {
  // FIXME: ???
  panic("conntectx not working");
  return 0;
}

int $darwin$close$NOCANCEL(int fd) {
  // FIXME: do we need to ensure anything else here?
  // See https://issues.chromium.org/issues/41033222
  return close(fd);
}

// Track 3 (docs/surface-audit/VERDICTS.md): Darwin nfds_t is 32-bit, Linux's is
// 64-bit -- the guest leaves the upper half of %rsi undefined and glibc reads it.
// The typed parameter here makes the compiler zero-extend. Event bits agree.
int $darwin$poll(void *fds, unsigned int nfds, int timeout) {
  return poll(fds, nfds, timeout);
}

// setbuf means setvbuf(f, buf, _IOFBF, BUFSIZ) -- and BUFSIZ is 1024 on Darwin vs
// 8192 in glibc, so forwarding overruns the guest's buffer by 7 KiB.
void $darwin$setbuf(FILE *f, char *buf) {
  setvbuf(f, buf, buf ? _IOFBF : _IONBF, buf ? 1024 : 0);
}

// termios (docs/surface-audit/VERDICTS.md): Darwin tcflag_t/speed_t are 64-bit
// with NCCS=20 and no c_line; glibc is 32-bit with c_line and NCCS=32, and the
// lflag bits + c_cc indexes are renumbered. Translated for the subset console
// code actually touches (echo/canonical/signals, VMIN/VTIME and friends);
// exotic serial-line flags are dropped. Headless runs never reach the tty path
// at all -- every fd is a pipe, so glibc's ENOTTY passes straight through.
struct $darwin$termios {
  uint64_t c_iflag, c_oflag, c_cflag, c_lflag;
  uint8_t c_cc[20];
  uint64_t c_ispeed, c_ospeed;
};

// (darwin_bit, linux_bit) pairs per flag word; iflag agrees except IXON/IXOFF.
static const uint64_t LFLAGS[][2] = {
    {0x8, 0x8},           // ECHO
    {0x100, 0x2},         // ICANON
    {0x80, 0x1},          // ISIG
    {0x2, 0x10},          // ECHOE
    {0x4, 0x20},          // ECHOK
    {0x10, 0x40},         // ECHONL
    {0x80000000u, 0x80},  // NOFLSH
    {0x400000, 0x100},    // TOSTOP
    {0x400, 0x8000},      // IEXTEN
};
static const uint64_t IFLAGS[][2] = {
    {0x1ff, 0x1ff},   // IGNBRK..ICRNL agree bit-for-bit
    {0x200, 0x400},   // IXON
    {0x400, 0x1000},  // IXOFF
    {0x800, 0x800},   // IXANY
};
// c_cc: (darwin_index, linux_index) for the characters that matter.
static const uint8_t CCMAP[][2] = {
    {0, 4},    // VEOF
    {3, 2},    // VERASE
    {5, 3},    // VKILL
    {8, 0},    // VINTR
    {9, 1},    // VQUIT
    {10, 10},  // VSUSP
    {16, 6},   // VMIN
    {17, 5},   // VTIME
};

int $darwin$tcgetattr(int fd, struct $darwin$termios *dt) {
  struct termios lt;
  if (tcgetattr(fd, &lt) != 0) return -1;
  memset(dt, 0, sizeof(*dt));
  for (unsigned i = 0; i < sizeof(LFLAGS) / sizeof(*LFLAGS); i++)
    if (lt.c_lflag & LFLAGS[i][1]) dt->c_lflag |= LFLAGS[i][0];
  for (unsigned i = 0; i < sizeof(IFLAGS) / sizeof(*IFLAGS); i++)
    if (lt.c_iflag & IFLAGS[i][1]) dt->c_iflag |= IFLAGS[i][0];
  if (lt.c_oflag & 0x1) dt->c_oflag |= 0x1;  // OPOST
  dt->c_cflag = 0x4b00;                      // CREAD|CS8, the only sane report
  for (unsigned i = 0; i < sizeof(CCMAP) / sizeof(*CCMAP); i++)
    dt->c_cc[CCMAP[i][0]] = lt.c_cc[CCMAP[i][1]];
  dt->c_ispeed = dt->c_ospeed = 38400;
  return 0;
}

int $darwin$tcsetattr(int fd, int act, const struct $darwin$termios *dt) {
  struct termios lt;
  if (tcgetattr(fd, &lt) != 0) return -1;  // start from current, overlay known bits
  for (unsigned i = 0; i < sizeof(LFLAGS) / sizeof(*LFLAGS); i++)
    lt.c_lflag = (dt->c_lflag & LFLAGS[i][0]) ? (lt.c_lflag | LFLAGS[i][1]) : (lt.c_lflag & ~LFLAGS[i][1]);
  for (unsigned i = 0; i < sizeof(IFLAGS) / sizeof(*IFLAGS); i++)
    lt.c_iflag = (dt->c_iflag & IFLAGS[i][0]) ? (lt.c_iflag | IFLAGS[i][1]) : (lt.c_iflag & ~IFLAGS[i][1]);
  for (unsigned i = 0; i < sizeof(CCMAP) / sizeof(*CCMAP); i++)
    lt.c_cc[CCMAP[i][1]] = dt->c_cc[CCMAP[i][0]];
  return tcsetattr(fd, act, &lt);  // TCSANOW/DRAIN/FLUSH 0/1/2 agree
}
