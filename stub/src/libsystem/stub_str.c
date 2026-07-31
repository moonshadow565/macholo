#include <ctype.h>
#include <string.h>
#include <sys/mman.h>

#include <common.h>

int $darwin$__tolower(int c) { return tolower(c); }

int $darwin$__toupper(int c) { return toupper(c); }

size_t $darwin$strlcpy(char *dst, const char *src, size_t dst_size) {
  size_t src_size = strlen(src);
  if (dst_size) {
    size_t n = (src_size + 1) < dst_size ? (src_size + 1) : dst_size;
    memcpy(dst, src, n - 1);
    dst[n - 1] = 0;
  }
  return src_size;
}

size_t $darwin$strlcat(char *dst, const char *src, size_t size) {
  if (!size) return 0;
  dst[size - 1] = 0;
  strncat(dst, src, size - strlen(dst) - 1);
  return strlen(dst);
}

int $darwin$vsnprintf_l(char *str, size_t size, locale_t loc, const char *format, va_list ap) {
  // used for sentry, don't really care
  return vsnprintf(str, size, format, ap);
}

void $darwin$__bzero(void *dst, size_t size) { memset(dst, 0, size); }

void memset_pattern16(void *b, const void *pattern16, size_t len) {
  char *dst = (char *)b;
  size_t rem = len;
  while (rem >= 16) {
    memcpy(dst, pattern16, 16);
    dst += 16;
    rem -= 16;
  }
  memcpy(dst, pattern16, rem);
}

void *$darwin$mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
  flags = (flags & 0x1f) | (flags & 0x1000 ? MAP_ANONYMOUS : 0);
  void *ret = mmap(addr, len, prot, flags, fd, offset);
  // printf("mmap(%p, %zx, %x, %x, %d, %zx) = %p\n", addr, len, prot, flags, fd, offset, ret);
  return ret;
}

// Track 3 (docs/surface-audit/VERDICTS.md): dlsym finds glibc's GNU basename
// (never strips trailing slashes) and glibc's dirname mutates its argument;
// Darwin's are POSIX-with-a-copy. Implemented directly on per-thread buffers.
static _Thread_local char basename_buf[1024] __attribute__((tls_model("initial-exec")));
static _Thread_local char dirname_buf[1024] __attribute__((tls_model("initial-exec")));

char *$darwin$basename(const char *path) {
  if (!path || !*path) {
    strcpy(basename_buf, ".");
    return basename_buf;
  }
  size_t end = strlen(path);
  while (end > 1 && path[end - 1] == '/') end--;
  if (end == 1 && path[0] == '/') {
    strcpy(basename_buf, "/");
    return basename_buf;
  }
  size_t start = end;
  while (start > 0 && path[start - 1] != '/') start--;
  size_t n = end - start;
  if (n >= sizeof(basename_buf)) n = sizeof(basename_buf) - 1;
  memcpy(basename_buf, path + start, n);
  basename_buf[n] = 0;
  return basename_buf;
}

char *$darwin$dirname(const char *path) {
  if (!path || !*path) {
    strcpy(dirname_buf, ".");
    return dirname_buf;
  }
  size_t end = strlen(path);
  while (end > 1 && path[end - 1] == '/') end--;
  while (end > 0 && path[end - 1] != '/') end--;  // drop last component
  while (end > 1 && path[end - 1] == '/') end--;  // drop separator run
  if (end == 0) {
    strcpy(dirname_buf, ".");
    return dirname_buf;
  }
  if (end >= sizeof(dirname_buf)) end = sizeof(dirname_buf) - 1;
  memcpy(dirname_buf, path, end);
  dirname_buf[end] = 0;
  return dirname_buf;
}

// strptime: prototype and return convention match; Darwin (FreeBSD-derived) and
// glibc differ on %Z outside known zone names, %p outside the C locale, and
// untouched-field behavior (VERDICTS.md INVESTIGATE). Routed through a shim so
// the divergence is a grep away if a guest ever trips on it.
extern char *strptime(const char *s, const char *format, void *tm);
char *$darwin$strptime(const char *s, const char *format, void *tm) {
  return strptime(s, format, tm);
}
