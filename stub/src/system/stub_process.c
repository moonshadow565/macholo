#include <common.h>

#define $darwin$MAXPATHLEN 1024

struct $darwin$proc_fileinfo {
  uint32_t fi_openflags;
  uint32_t fi_status;
  off_t fi_offset;
  int32_t fi_type;
  uint32_t fi_guardflags;
};

struct $darwin$vinfo_stat {
  uint32_t vst_dev;          /* [XSI] ID of device containing file */
  uint16_t vst_mode;         /* [XSI] Mode of file (see below) */
  uint16_t vst_nlink;        /* [XSI] Number of hard links */
  uint64_t vst_ino;          /* [XSI] File serial number */
  uid_t vst_uid;             /* [XSI] User ID of the file */
  gid_t vst_gid;             /* [XSI] Group ID of the file */
  int64_t vst_atime;         /* [XSI] Time of last access */
  int64_t vst_atimensec;     /* nsec of last access */
  int64_t vst_mtime;         /* [XSI] Last data modification time */
  int64_t vst_mtimensec;     /* last data modification nsec */
  int64_t vst_ctime;         /* [XSI] Time of last status change */
  int64_t vst_ctimensec;     /* nsec of last status change */
  int64_t vst_birthtime;     /*  File creation time(birth)  */
  int64_t vst_birthtimensec; /* nsec of File creation time */
  off_t vst_size;            /* [XSI] file size, in bytes */
  int64_t vst_blocks;        /* [XSI] blocks allocated for file */
  int32_t vst_blksize;       /* [XSI] optimal blocksize for I/O */
  uint32_t vst_flags;        /* user defined flags for file */
  uint32_t vst_gen;          /* file generation number */
  uint32_t vst_rdev;         /* [XSI] Device ID */
  int64_t vst_qspare[2];     /* RESERVED: DO NOT USE! */
};

struct $darwin$psem_info {
  struct $darwin$vinfo_stat psem_stat;
  char psem_name[$darwin$MAXPATHLEN];
};

struct $darwin$psem_fdinfo {
  struct $darwin$proc_fileinfo pfi;
  struct $darwin$psem_info pseminfo;
};

#define PROC_PIDFDPSEMINFO 4
#define PROC_PIDFDPSEMINFO_SIZE (sizeof(struct $darwin$psem_fdinfo), 1184)

void $darwin$proc_listpidspath() {
  // TODO
  panic("proc_listpidspath");
}

int $darwin$proc_pidinfo(int pid, int flavor, int fd, void* buffer, uint32_t buffersize, int32_t* retval) {
  if (flavor == 4) {
    return 0;
  }
  // TODO
  panic("proc_pidinfo");
}

void $darwin$proc_pidpath() {
  // TODO
  panic("proc_pidpath");
}

__pid_t $darwin$fork() {
  panic("fork not supported!");
  return -1;
}

int $darwin$posix_spawn_file_actions_addinherit_np(void* file_actions, int filedes) {
  // NOTE: used by crashpad, don't care
  panic("posix_spawn_file_actions_addinherit_np");
  return 0;
}

int $darwin$posix_spawn_file_actions_destroy() {
  // NOTE: used by crashpad, don't care
  panic("posix_spawn_file_actions_destroy");
  return 0;
}

int $darwin$posix_spawn_file_actions_init() {
  // NOTE: used by crashpad, don't care
  panic("posix_spawn_file_actions_init");
  return 0;
}
