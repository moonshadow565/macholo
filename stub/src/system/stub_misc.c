#include <common.h>
#include <pwd.h>

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
