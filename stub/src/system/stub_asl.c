#include <common.h>

// Logging library, don't really care about any of those

typedef struct aslclient_s *aslclient;
typedef struct aslmsg_s *aslmsg;

void asl_close(aslclient asl) {
  // NOTE: don't care
  free(asl);
}

void asl_free(aslmsg msg) {
  // NOTE: don't care
  free(msg);
}

aslmsg asl_new(uint32_t type) {
  // NOTE: don't care
  return (aslmsg)malloc(8);
}

aslclient asl_open(const char *ident, const char *facility, uint32_t opt) {
  // NOTE: don't care
  return (aslclient)malloc(8);
}

int asl_send(aslclient asl, aslmsg msg) {
  // NOTE: don't care, allways success
  return 0;
}

int asl_set(aslmsg msg, const char *key, const char *value) {
  // NOTE: don't care, allways success
  return 0;
}
