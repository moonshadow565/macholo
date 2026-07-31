#define __USE_LARGEFILE64 1
#define _GNU_SOURCE 1
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/file.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <common.h>

typedef int32_t $darwin$dev_t;
typedef uint16_t $darwin$mode_t;
typedef uint16_t $darwin$nlink_t;
typedef uint64_t $darwin$ino64_t;
typedef uint32_t $darwin$uid_t;
typedef uint32_t $darwin$gid_t;
typedef uint32_t $darwin$sigset_t;
typedef struct timespec $darwin$timespec;

struct $darwin$dirent64 {
  uint64_t d_ino;
  uint64_t d_seekoff;
  uint16_t d_reclen;
  uint16_t d_namlen;
  uint8_t d_type;
  char d_name[1024];
};

struct $darwin$DIR {
  void *real;
  struct $darwin$dirent64 tmp;
};

struct $darwin$stat64 {
  $darwin$dev_t st_dev;
  $darwin$mode_t st_mode;
  $darwin$nlink_t st_nlink;
  $darwin$ino64_t st_ino;
  $darwin$uid_t st_uid;
  $darwin$gid_t st_gid;
  $darwin$dev_t st_rdev;
  $darwin$timespec st_atimespec;
  $darwin$timespec st_mtimespec;
  $darwin$timespec st_ctimespec;
  $darwin$timespec st_birthtimespec;
  off_t st_size;
  blkcnt_t st_blocks;
  blksize_t st_blksize;
  uint32_t st_flags;
  uint32_t st_gen;
  int32_t st_lspare;
};

static void stat64_from_linux(struct $darwin$stat64 *into, struct stat64 *from) {
  into->st_dev = ($darwin$dev_t)from->st_dev;
  into->st_mode = ($darwin$mode_t)from->st_mode;
  into->st_nlink = ($darwin$nlink_t)from->st_nlink;
  into->st_ino = ($darwin$ino64_t)from->st_ino;
  into->st_uid = ($darwin$uid_t)from->st_uid;
  into->st_gid = ($darwin$gid_t)from->st_gid;
  into->st_rdev = ($darwin$dev_t)from->st_rdev;
  into->st_atimespec = from->st_atim;
  into->st_mtimespec = from->st_mtim;
  into->st_ctimespec = from->st_ctim;
  into->st_size = from->st_size;
  into->st_blocks = from->st_blocks;
  into->st_blksize = from->st_blksize;
}

static void dirent64_from_linux(struct $darwin$dirent64 *into, struct dirent64 *from) {
  into->d_ino = from->d_ino;
  into->d_reclen = from->d_reclen;
  into->d_type = from->d_type;
  into->d_namlen = strlen(from->d_name);
  into->d_seekoff = 0;
  strcpy(into->d_name, from->d_name);
}

// Darwin glob_t: {size_t gl_pathc; int gl_matchc; size_t gl_offs; int gl_flags;
// char **gl_pathv; <6 ALTDIRFUNC fn pointers>} -- glibc's glob64_t has gl_pathv
// second. GLOB_* flag values are permuted too. Results are produced by glibc's
// glob64 into a scratch struct and the fields copied over; globfree below
// reconstructs the glibc view to free them (globfree64 only reads
// pathc/pathv/offs/flags). GLOB_ALTDIRFUNC is refused -- the guest would supply
// Darwin-layout dirent callbacks.
struct $darwin$glob_t {
  size_t gl_pathc;
  int gl_matchc;
  size_t gl_offs;
  int gl_flags;
  char **gl_pathv;
  void *gl_fns[6];
};

static int glob_flags_to_gnu(int f) {
  int l = 0;
  if (f & 0x0001) l |= GLOB_APPEND;
  if (f & 0x0002) l |= GLOB_DOOFFS;
  if (f & 0x0004) l |= GLOB_ERR;
  if (f & 0x0008) l |= GLOB_MARK;
  if (f & 0x0010) l |= GLOB_NOCHECK;
  if (f & 0x0020) l |= GLOB_NOSORT;
  if (f & 0x0080) l |= GLOB_BRACE;
  if (f & 0x0800) l |= GLOB_TILDE;
  if (f & 0x2000) l |= GLOB_NOESCAPE;
  return l;
}

int $darwin$glob$INODE64(const char *pattern, int flags, int (*errfunc)(const char *, int), struct $darwin$glob_t *pglob) {
  if (flags & 0x0040) panic("glob: GLOB_ALTDIRFUNC from guest");
  glob64_t g = {0};
  g.gl_offs = pglob->gl_offs;
  if (flags & 0x0001) {  // GLOB_APPEND: rebuild the glibc view of prior results
    g.gl_pathc = pglob->gl_pathc;
    g.gl_pathv = pglob->gl_pathv;
    g.gl_flags = glob_flags_to_gnu(pglob->gl_flags);
  }
  int r = glob64(pattern, glob_flags_to_gnu(flags), errfunc, &g);
  pglob->gl_pathc = g.gl_pathc;
  pglob->gl_matchc = (int)g.gl_pathc;
  pglob->gl_offs = g.gl_offs;
  pglob->gl_flags = flags;
  pglob->gl_pathv = g.gl_pathv;
  return r;  // 0 / GLOB_NOSPACE(1) / GLOB_ABORTED(2) / GLOB_NOMATCH(3) agree
}

// TODO: provide more complete filesystem emulation/overlay ?
struct $darwin$DIR *$darwin$opendir$INODE64(const char *name) {
  DIR *real = opendir(name);
  if (!real) return NULL;

  struct $darwin$DIR *stream = (struct $darwin$DIR *)malloc(sizeof(struct $darwin$DIR));
  stream->real = real;
  return stream;
}

int $darwin$closedir(struct $darwin$DIR *stream) {
  if (!stream) return 0;
  int e = closedir((DIR *)stream->real);
  if (e) return e;
  free(stream);
  return 0;
}

struct $darwin$dirent64 *$darwin$readdir$INODE64(struct $darwin$DIR *dirp) {
  struct dirent64 *from = readdir64((DIR *)dirp->real);
  if (!from) return NULL;
  dirent64_from_linux(&dirp->tmp, from);
  return &dirp->tmp;
}

int $darwin$stat$INODE64(char const *path, struct $darwin$stat64 *s) {
  if (!s) return stat64(path, (struct stat64 *)s);
  struct stat64 tmp;
  int ret = stat64(path, &tmp);
  stat64_from_linux(s, &tmp);
  return ret;
}

int $darwin$lstat$INODE64(char const *path, struct $darwin$stat64 *s) {
  if (!s) return lstat64(path, (struct stat64 *)s);
  struct stat64 tmp;
  int ret = lstat64(path, &tmp);
  stat64_from_linux(s, &tmp);
  return ret;
}

int $darwin$fstat$INODE64(int fd, struct $darwin$stat64 *s) {
  if (!s) return fstat64(fd, (struct stat64 *)s);
  struct stat64 tmp;
  int ret = fstat64(fd, &tmp);
  stat64_from_linux(s, &tmp);
  return ret;
}

char *$darwin$realpath$DARWIN_EXTSN(const char *name, char *resolved_name) {
  // FIXME: Defining _DARWIN_C_SOURCE or _DARWIN_BETTER_REALPATH before including stdlib.h
  /// will cause the provided implementation of realpath() to use F_GETPATH from fcntl(2) to discover the path.
  return realpath(name, resolved_name);
}

#include <unistd.h>
// FILE* $darwin$fopen(char const* path, char const* mode) {
//   char dir[PATH_MAX];
//   getcwd(dir, sizeof(dir));
//   printf("opening %s with %s from %s\n", path, mode, dir);
//   return fopen(path, mode);
// }

int $darwin$chdir(const char* path) {
  // workaround stupidity
  if (strcmp(path, ".") == 0 || strcmp(path, "../../..") == 0)
    return 0;
  return chdir(path);
}

// copyfile(3) -- Darwin's file copy helper. We only ever need plain data copies.
typedef void *copyfile_state_t;
static char copyfile_state_handle[1];

copyfile_state_t copyfile_state_alloc(void) { return copyfile_state_handle; }

int copyfile_state_free(copyfile_state_t s) { return 0; }

int copyfile(const char *from, const char *to, copyfile_state_t state, uint32_t flags) {
  if (!from || !to) return -1;
  int in = open(from, O_RDONLY);
  if (in < 0) return -1;
  struct stat64 st;
  if (fstat64(in, &st) != 0) st.st_mode = 0644;
  int out = open(to, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
  if (out < 0) {
    close(in);
    return -1;
  }
  int ret = 0;
  char buf[65536];
  for (;;) {
    ssize_t n = read(in, buf, sizeof(buf));
    if (n == 0) break;
    if (n < 0 || write(out, buf, (size_t)n) != n) {
      ret = -1;
      break;
    }
  }
  close(in);
  close(out);
  return ret;
}

// Darwin's struct statfs (the 64-bit-inode variant the game links against). Only the
// free/total block counts matter -- the game uses them for a disk-space check before
// writing settings/caches.
#define $darwin$MFSTYPENAMELEN 16
#define $darwin$MAXPATHLEN 1024

struct $darwin$statfs64 {
  uint32_t f_bsize;
  int32_t f_iosize;
  uint64_t f_blocks;
  uint64_t f_bfree;
  uint64_t f_bavail;
  uint64_t f_files;
  uint64_t f_ffree;
  int32_t f_fsid[2];
  $darwin$uid_t f_owner;
  uint32_t f_type;
  uint32_t f_flags;
  uint32_t f_fssubtype;
  char f_fstypename[$darwin$MFSTYPENAMELEN];
  char f_mntonname[$darwin$MAXPATHLEN];
  char f_mntfromname[$darwin$MAXPATHLEN];
  uint32_t f_reserved[8];
};

int $darwin$statfs$INODE64(char const *path, struct $darwin$statfs64 *s) {
  struct statvfs tmp;
  if (statvfs(path, &tmp) != 0) return -1;
  if (!s) return 0;
  memset(s, 0, sizeof(*s));
  s->f_bsize = (uint32_t)tmp.f_bsize;
  s->f_iosize = (int32_t)tmp.f_bsize;
  s->f_blocks = tmp.f_blocks;
  s->f_bfree = tmp.f_bfree;
  s->f_bavail = tmp.f_bavail;
  s->f_files = tmp.f_files;
  s->f_ffree = tmp.f_ffree;
  s->f_owner = ($darwin$uid_t)getuid();
  strncpy(s->f_fstypename, "hfs", sizeof(s->f_fstypename) - 1);
  strncpy(s->f_mntonname, "/", sizeof(s->f_mntonname) - 1);
  strncpy(s->f_mntfromname, path, sizeof(s->f_mntfromname) - 1);
  return 0;
}

// Track 3 (docs/surface-audit/VERDICTS.md): open() was the headline yolo-lane bug --
// Darwin O_CREAT=0x200/O_TRUNC=0x400 land on Linux as O_TRUNC/O_APPEND with no
// O_CREAT at all. Full flag translation; O_SHLOCK/O_EXLOCK emulate via flock.
int $darwin$open(const char *path, int oflag, ...) {
  int mode = 0;
  if (oflag & 0x200) {
    va_list ap;
    va_start(ap, oflag);
    mode = va_arg(ap, int);
    va_end(ap);
  }
  int l = oflag & O_ACCMODE;  // low two bits agree
  if (oflag & 0x0004) l |= O_NONBLOCK;
  if (oflag & 0x0008) l |= O_APPEND;
  if (oflag & 0x0080) l |= O_SYNC;
  if (oflag & 0x0100) l |= O_NOFOLLOW;
  if (oflag & 0x0200) l |= O_CREAT;
  if (oflag & 0x0400) l |= O_TRUNC;
  if (oflag & 0x0800) l |= O_EXCL;
  if (oflag & 0x20000) l |= O_NOCTTY;
  if (oflag & 0x100000) l |= O_DIRECTORY;
  if (oflag & 0x200000) l |= O_NOFOLLOW;  // O_SYMLINK: closest Linux behavior
  if (oflag & 0x1000000) l |= O_CLOEXEC;
  int fd = open(path, l, mode);
  if (fd >= 0 && (oflag & 0x30)) flock(fd, (oflag & 0x20) ? LOCK_EX : LOCK_SH);
  return fd;
}

// fcntl: commands 0..4 agree, the lock/owner families are renumbered, struct flock
// is reordered (BSD: start,len,pid,type,whence vs Linux: type,whence,start,len,pid)
// and F_RDLCK/F_WRLCK values differ. Darwin-only commands get local handling.
struct $darwin$flock {
  int64_t l_start;
  int64_t l_len;
  int32_t l_pid;
  int16_t l_type;   // F_RDLCK=1 F_UNLCK=2 F_WRLCK=3
  int16_t l_whence;
};

static short lock_type_to_gnu(short t) { return t == 1 ? F_RDLCK : t == 3 ? F_WRLCK : F_UNLCK; }
static short lock_type_to_darwin(short t) { return t == F_RDLCK ? 1 : t == F_WRLCK ? 3 : 2; }

static int oflags_to_gnu(int f) {
  int l = f & O_ACCMODE;
  if (f & 0x4) l |= O_NONBLOCK;
  if (f & 0x8) l |= O_APPEND;
  if (f & 0x40) l |= O_ASYNC;
  return l;
}

static int oflags_to_darwin(int l) {
  int f = l & O_ACCMODE;
  if (l & O_NONBLOCK) f |= 0x4;
  if (l & O_APPEND) f |= 0x8;
  if (l & O_ASYNC) f |= 0x40;
  if (l & O_CREAT) f |= 0x200;
  return f;
}

int $darwin$fcntl(int fd, int cmd, ...) {
  va_list ap;
  va_start(ap, cmd);
  void *arg = va_arg(ap, void *);
  va_end(ap);
  switch (cmd) {
    case 0:  // F_DUPFD
    case 1:  // F_GETFD
    case 2:  // F_SETFD (FD_CLOEXEC=1 agrees)
      return fcntl(fd, cmd, arg);
    case 3:  // F_GETFL
      return oflags_to_darwin(fcntl(fd, F_GETFL));
    case 4:  // F_SETFL
      return fcntl(fd, F_SETFL, oflags_to_gnu((int)(intptr_t)arg));
    case 5:  // F_GETOWN
      return fcntl(fd, F_GETOWN);
    case 6:  // F_SETOWN
      return fcntl(fd, F_SETOWN, arg);
    case 7:    // F_GETLK
    case 8:    // F_SETLK
    case 9: {  // F_SETLKW
      struct $darwin$flock *dfl = arg;
      struct flock lfl = {
          .l_type = lock_type_to_gnu(dfl->l_type),
          .l_whence = dfl->l_whence,
          .l_start = dfl->l_start,
          .l_len = dfl->l_len,
          .l_pid = dfl->l_pid,
      };
      int r = fcntl(fd, cmd == 7 ? F_GETLK : cmd == 8 ? F_SETLK : F_SETLKW, &lfl);
      if (r == 0 && cmd == 7) {
        dfl->l_type = lock_type_to_darwin(lfl.l_type);
        dfl->l_whence = lfl.l_whence;
        dfl->l_start = lfl.l_start;
        dfl->l_len = lfl.l_len;
        dfl->l_pid = lfl.l_pid;
      }
      return r;
    }
    case 67:  // F_DUPFD_CLOEXEC
      return fcntl(fd, F_DUPFD_CLOEXEC, arg);
    case 48:  // F_NOCACHE: no Linux equivalent, direct I/O toggle -- accept and ignore
      return 0;
    case 51:  // F_FULLFSYNC
      return fsync(fd);
    case 50: {  // F_GETPATH: fill caller's MAXPATHLEN(1024) buffer
      char proc[64];
      snprintf(proc, sizeof(proc), "/proc/self/fd/%d", fd);
      ssize_t n = readlink(proc, arg, 1023);
      if (n < 0) return -1;
      ((char *)arg)[n] = 0;
      return 0;
    }
    case 44: {  // F_RDADVISE { off_t offset; int count; }
      struct { int64_t offset; int count; } *ra = arg;
      return posix_fadvise(fd, ra->offset, ra->count, POSIX_FADV_WILLNEED) ? -1 : 0;
    }
    case 73:  // F_SETNOSIGPIPE: per-fd SIGPIPE suppression; the socket shims send
      return 0;  // with MSG_NOSIGNAL and SIGPIPE is ignored process-wide -- no-op
    case 74:  // F_GETNOSIGPIPE
      return 0;
    default:
      fprintf(stderr, "[fcntl] unhandled darwin cmd: %d\n", cmd);
      *__errno_location() = EINVAL;
      return -1;
  }
}

// utimensat: AT_FDCWD is -2 on Darwin vs -100 on Linux; AT_SYMLINK_NOFOLLOW is
// 0x20 vs 0x100. UTIME_NOW/UTIME_OMIT values agree, struct timespec is identical.
int $darwin$utimensat(int fd, const char *path, const struct timespec times[2], int flag) {
  return utimensat(fd == -2 ? AT_FDCWD : fd, path, times, (flag & 0x20) ? AT_SYMLINK_NOFOLLOW : 0);
}

// xattr family: Darwin appends (position, options) args Linux doesn't have --
// glibc reads 'position' as 'flags'. XATTR_NOFOLLOW(1) selects the l-variants;
// XATTR_CREATE/REPLACE (2/4) map to Linux's 1/2.
ssize_t $darwin$getxattr(const char *path, const char *name, void *value, size_t size, uint32_t position, int options) {
  return (options & 1) ? lgetxattr(path, name, value, size) : getxattr(path, name, value, size);
}

int $darwin$setxattr(const char *path, const char *name, const void *value, size_t size, uint32_t position, int options) {
  int flags = ((options & 2) ? XATTR_CREATE : 0) | ((options & 4) ? XATTR_REPLACE : 0);
  return (options & 1) ? lsetxattr(path, name, value, size, flags) : setxattr(path, name, value, size, flags);
}

int $darwin$removexattr(const char *path, const char *name, int options) {
  return (options & 1) ? lremovexattr(path, name) : removexattr(path, name);
}

// Paired with $darwin$glob$INODE64: reconstruct the glibc view of the results
// it allocated and let globfree64 release them.
void $darwin$globfree(struct $darwin$glob_t *pglob) {
  glob64_t g = {0};
  g.gl_pathc = pglob->gl_pathc;
  g.gl_pathv = pglob->gl_pathv;
  g.gl_offs = pglob->gl_offs;
  g.gl_flags = glob_flags_to_gnu(pglob->gl_flags);
  globfree64(&g);
  pglob->gl_pathv = NULL;
  pglob->gl_pathc = 0;
}
