#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <common.h>

typedef struct mach_timebase_info {
  uint32_t numer;
  uint32_t denom;
} *mach_timebase_info_t;

typedef int clock_res_t;

typedef struct mach_timespec {
  unsigned int tv_sec;
  unsigned int tv_nsec;
} mach_timespec_t;

typedef int kern_return_t;

typedef intptr_t task_t;

typedef uint32_t semaphore_t;

// NOTE: used by crashpad, don't really care?
// FIXME: better definition
intptr_t $darwin$NDR_record = 0;

task_t $darwin$mach_task_self_ = -1;

// TODO: make a somewhat better stack protector?
long $darwin$__stack_chk_guard[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int $darwin$Gestalt(int select, int32_t *response) {
  // NOTE: used for version reporting
  // TODO: implement 'sys1', 'sys2', 'sys3'
  return 0;
}

int $darwin$__darwin_check_fd_set_overflow(int n, const void *fd_set, int unlimited_select) {
  if (n >= FD_SETSIZE) {
    // FIXME: actually scan stack?
    // panic("__darwin_check_fd_set_overflow");
  }
  return 1;
}

int *$darwin$__error() {
  // NOTE: this is correct
  return __errno_location();
}

// DATA: the task's bootstrap server port. There is no launchd here.
uint32_t bootstrap_port = 0;

kern_return_t host_get_exception_ports(void *host_priv, int mask, uint32_t *masks, unsigned *masksCnt, uint32_t *old_handlers, int *old_behaviors, int *old_flavors) {
  // Report "no handlers installed" rather than failing: callers save the old
  // ports so they can chain to them, and an empty list means "nothing to chain to".
  if (masksCnt) *masksCnt = 0;
  return KERN_SUCCESS;
}

kern_return_t host_set_exception_ports(void *host_priv, int mask, uint32_t new_port, int behavior, int flavor) {
  return KERN_SUCCESS;
}

// mach's vm_statistics64_data_t, in the layout host_statistics64(HOST_VM_INFO64) fills.
struct vm_statistics64 {
  uint32_t free_count;
  uint32_t active_count;
  uint32_t inactive_count;
  uint32_t wire_count;
  uint64_t zero_fill_count;
  uint64_t reactivations;
  uint64_t pageins;
  uint64_t pageouts;
  uint64_t faults;
  uint64_t cow_faults;
  uint64_t lookups;
  uint64_t hits;
  uint64_t purges;
  uint32_t purgeable_count;
  uint32_t speculative_count;
  uint64_t decompressions;
  uint64_t compressions;
  uint64_t swapins;
  uint64_t swapouts;
  uint32_t compressor_page_count;
  uint32_t throttled_count;
  uint32_t external_page_count;
  uint32_t internal_page_count;
  uint64_t total_uncompressed_pages_in_compressor;
};

#define HOST_LOAD_INFO 1
#define HOST_VM_INFO 2
#define HOST_CPU_LOAD_INFO 3
#define HOST_VM_INFO64 4

// Reports real page counts. Returning zeroes here makes the client believe the machine
// has no memory at all, which is a bad starting point for any streaming budget.
static void fill_vm_statistics(struct vm_statistics64 *out) {
  memset(out, 0, sizeof(*out));
  long const free_pages = sysconf(_SC_AVPHYS_PAGES);
  long const total_pages = sysconf(_SC_PHYS_PAGES);
  out->free_count = free_pages > 0 ? (uint32_t)free_pages : 0;
  if (total_pages > 0 && free_pages > 0 && total_pages > free_pages) {
    out->active_count = (uint32_t)((total_pages - free_pages) / 2);
    out->inactive_count = (uint32_t)((total_pages - free_pages) / 4);
    out->wire_count = (uint32_t)((total_pages - free_pages) / 4);
  }
}

int host_statistics(void *host_priv, int flavor, void *host_info_out, unsigned *host_info_outCnt) {
  if (!host_info_out || !host_info_outCnt) return KERN_INVALID_ARGUMENT;
  size_t const size = (size_t)*host_info_outCnt * 4;
  memset(host_info_out, 0, size);
  if (flavor == HOST_VM_INFO) {
    struct vm_statistics64 stats;
    fill_vm_statistics(&stats);
    // 32-bit vm_statistics shares the first four counters with the 64-bit variant.
    memcpy(host_info_out, &stats, size < 16 ? size : 16);
  }
  return KERN_SUCCESS;
}

int host_statistics64(void *host_priv, int flavor, void *host_info_out, unsigned *host_info_outCnt) {
  if (!host_info_out || !host_info_outCnt) return KERN_INVALID_ARGUMENT;
  size_t const size = (size_t)*host_info_outCnt * 4;
  memset(host_info_out, 0, size);
  if (flavor == HOST_VM_INFO64) {
    struct vm_statistics64 stats;
    fill_vm_statistics(&stats);
    memcpy(host_info_out, &stats, size < sizeof(stats) ? size : sizeof(stats));
  }
  return KERN_SUCCESS;
}

int host_page_size(void *host, uintptr_t *out_page_size) {
  if (out_page_size) *out_page_size = 4096;
  return KERN_SUCCESS;
}

kern_return_t host_swap_exception_ports(void *host_priv, int mask, uint32_t new_port, int behavior, int flavor, uint32_t *masks, unsigned *masksCnt, uint32_t *old_handlers, int *old_behaviors, int *old_flavors) {
  // Report "no handlers installed" rather than failing: callers save the old
  // ports so they can chain to them, and an empty list means "nothing to chain to".
  if (masksCnt) *masksCnt = 0;
  return KERN_SUCCESS;
}

void mach_error_string() {
  // TODO
  panic("mach_error_string");
}

intptr_t mach_host_self() {
  // I dont care really.
  return 1;
}

// No mach IPC exists. MACH_RCV_TIMED_OUT tells the caller "nothing arrived", which is
// the one answer a message pump can loop on forever without misbehaving.
kern_return_t mach_msg(void *msg, int option, uint32_t send_size, uint32_t rcv_size, uint32_t rcv_name,
                       uint32_t timeout, uint32_t notify) {
  return 0x10004003; /* MACH_RCV_TIMED_OUT */
}

void mach_msg_destroy(void *msg) {
}

// Mach ports are handed out as opaque increasing numbers. Nothing here implements
// message passing, but the anti-tamper code allocates a port for its exception handler
// and only ever passes the name back to other (equally inert) mach calls, so a unique
// non-zero token is all it needs.
kern_return_t mach_port_allocate(task_t task, int right, uint32_t *name) {
  static uint32_t next = 0x1000;
  if (!name) return KERN_INVALID_ARGUMENT;
  *name = __atomic_add_fetch(&next, 4, __ATOMIC_RELAXED);
  return KERN_SUCCESS;
}

kern_return_t mach_port_deallocate(task_t task, uint32_t name) {
  return KERN_SUCCESS;
}

kern_return_t mach_port_extract_right(task_t task, uint32_t name, int msgt_name, uint32_t *poly, int *polyPoly) {
  if (poly) *poly = name;
  if (polyPoly) *polyPoly = msgt_name;
  return KERN_SUCCESS;
}

kern_return_t mach_port_mod_refs(task_t task, uint32_t name, int right, int delta) {
  return KERN_SUCCESS;
}

kern_return_t mach_port_request_notification(task_t task, uint32_t name, int msgid, uint32_t sync, uint32_t notify,
                                             int notifyPoly, uint32_t *previous) {
  if (previous) *previous = 0;
  return KERN_SUCCESS;
}

// A per-thread token. Nothing can be done with a "thread port" here, but it must be
// unique per thread and stable, which pthread_self already is.
uint32_t mach_thread_self(void) {
  return (uint32_t)(uintptr_t)pthread_self();
}

int mach_timebase_info(mach_timebase_info_t info) {
  if (!info) return KERN_INVALID_ARGUMENT;
  // NOTE: pretend that absolute time is allways in nanoseconds
  info->numer = 1;
  info->denom = 1;
  return KERN_SUCCESS;
}

uint64_t mach_absolute_time() {
  // return clock();
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_nsec + ts.tv_sec * 1000000000;
}

#define SEM_TABLE_MAX 4096
// FIXME: replace lock with atomic
static pthread_mutex_t _sem_table_lock = PTHREAD_MUTEX_INITIALIZER;
static sem_t _sem[SEM_TABLE_MAX] = {0};
static _Bool _sem_bits[SEM_TABLE_MAX] = {0};

__attribute__((constructor)) static void _sem_lock_init() { pthread_mutex_init(&_sem_table_lock, NULL); }

kern_return_t $darwin$semaphore_create(task_t task, semaphore_t *semaphore, int policy, int value) {
  if (!semaphore) return KERN_INVALID_ARGUMENT;
  pthread_mutex_lock(&_sem_table_lock);
  for (int i = 1; i < SEM_TABLE_MAX; ++i) {
    if (!_sem_bits[i]) {
      int result = sem_init(&_sem[i], 0, value);
      if (!result) {
        pthread_mutex_unlock(&_sem_table_lock);
        _sem_bits[i] = 1;
        *semaphore = i;
        return KERN_SUCCESS;
      }
      break;
    }
  }
  pthread_mutex_unlock(&_sem_table_lock);
  return KERN_NOT_SUPPORTED;
}

kern_return_t $darwin$semaphore_destroy(task_t task, semaphore_t semaphore) {
  if (!semaphore) return KERN_SUCCESS;
  if (semaphore < 1 || semaphore >= SEM_TABLE_MAX || !_sem_bits[semaphore]) return KERN_SEMAPHORE_DESTROYED;

  pthread_mutex_lock(&_sem_table_lock);
  sem_close(&_sem[semaphore]);
  _sem_bits[semaphore] = 0;
  pthread_mutex_unlock(&_sem_table_lock);
  return KERN_SUCCESS;
}

static kern_return_t sem_convert_result(int result) {
  if (result == 0) return KERN_SUCCESS;
  result = errno;
  switch (result) {
    case 0:
      return KERN_SUCCESS;
    case EACCES:
      return KERN_PROTECTION_FAILURE;
    case ETIMEDOUT:
      return KERN_OPERATION_TIMED_OUT;
    case EINTR:
      return KERN_ABORTED;
    case EINVAL:
      return KERN_INVALID_ARGUMENT;
    case EAGAIN:
      return KERN_ABORTED;
    default:
      return KERN_NOT_SUPPORTED;
  }
}

kern_return_t $darwin$semaphore_signal(semaphore_t semaphore) {
  if (semaphore < 1 || semaphore >= SEM_TABLE_MAX || !_sem_bits[semaphore]) {
    panic("bad!");
    return KERN_SEMAPHORE_DESTROYED;
  }
  int result = sem_post(&_sem[semaphore]);
  return sem_convert_result(result);
}

kern_return_t $darwin$semaphore_wait(semaphore_t semaphore) {
  if (semaphore < 1 || semaphore >= SEM_TABLE_MAX || !_sem_bits[semaphore]) {
    panic("bad!");
    return KERN_SEMAPHORE_DESTROYED;
  }
  int result = sem_wait(&_sem[semaphore]);
  return sem_convert_result(result);
}

kern_return_t $darwin$semaphore_timedwait(semaphore_t semaphore, mach_timespec_t wait_time) {
  if (semaphore < 1 || semaphore >= SEM_TABLE_MAX || !_sem_bits[semaphore]) {
    panic("bad!");
    return KERN_SEMAPHORE_DESTROYED;
  }

  // TODO: check 0 case
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += wait_time.tv_sec;
  ts.tv_nsec += wait_time.tv_nsec;
  if (ts.tv_nsec >= 1000000000) {
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
  }
  int result = sem_timedwait(&_sem[semaphore], &ts);
  return sem_convert_result(result);
}

kern_return_t task_get_exception_ports(task_t task, int mask, uint32_t *masks, unsigned *masksCnt, uint32_t *old_handlers, int *old_behaviors, int *old_flavors) {
  // Report "no handlers installed" rather than failing: callers save the old
  // ports so they can chain to them, and an empty list means "nothing to chain to".
  if (masksCnt) *masksCnt = 0;
  return KERN_SUCCESS;
}

int task_info(intptr_t target_task, unsigned flavor, void *task_info_out, unsigned *task_info_outCn) {
  if (flavor == 22) {
    // used to log hardware statistics, i do not really care
    memset(task_info_out, 0, *task_info_outCn);
    return KERN_SUCCESS;
  }
  // TODO
  panic("task_info");
}

kern_return_t task_set_exception_ports(task_t task, int mask, uint32_t new_port, int behavior, int flavor) {
  return KERN_SUCCESS;
}

kern_return_t task_swap_exception_ports(task_t task, int mask, uint32_t new_port, int behavior, int flavor, uint32_t *masks, unsigned *masksCnt, uint32_t *old_handlers, int *old_behaviors, int *old_flavors) {
  // Report "no handlers installed" rather than failing: callers save the old
  // ports so they can chain to them, and an empty list means "nothing to chain to".
  if (masksCnt) *masksCnt = 0;
  return KERN_SUCCESS;
}

kern_return_t thread_get_exception_ports(uint32_t thread, int mask, uint32_t *masks, unsigned *masksCnt, uint32_t *old_handlers, int *old_behaviors, int *old_flavors) {
  // Report "no handlers installed" rather than failing: callers save the old
  // ports so they can chain to them, and an empty list means "nothing to chain to".
  if (masksCnt) *masksCnt = 0;
  return KERN_SUCCESS;
}

kern_return_t thread_set_exception_ports(uint32_t thread, int mask, uint32_t new_port, int behavior, int flavor) {
  return KERN_SUCCESS;
}

kern_return_t thread_swap_exception_ports(uint32_t thread, int mask, uint32_t new_port, int behavior, int flavor, uint32_t *masks, unsigned *masksCnt, uint32_t *old_handlers, int *old_behaviors, int *old_flavors) {
  // Report "no handlers installed" rather than failing: callers save the old
  // ports so they can chain to them, and an empty list means "nothing to chain to".
  if (masksCnt) *masksCnt = 0;
  return KERN_SUCCESS;
}

#define VM_FLAGS_ANYWHERE 0x0001

// Plain page allocation, which is exactly mmap. WC3's unpacker allocates scratch space
// through this, so it has to work rather than trap.
kern_return_t vm_allocate(task_t task, uintptr_t *address, size_t size, int flags) {
  if (!address || !size) return KERN_INVALID_ARGUMENT;
  void *want = (flags & VM_FLAGS_ANYWHERE) ? NULL : (void *)*address;
  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS | ((flags & VM_FLAGS_ANYWHERE) ? 0 : MAP_FIXED);
  void *got = mmap(want, size, PROT_READ | PROT_WRITE, mmap_flags, -1, 0);
  if (got == MAP_FAILED) return KERN_NO_SPACE;
  *address = (uintptr_t)got;
  return KERN_SUCCESS;
}

kern_return_t vm_deallocate(task_t task, uintptr_t address, size_t size) {
  if (!address || !size) return KERN_SUCCESS;
  return munmap((void *)address, size) == 0 ? KERN_SUCCESS : KERN_INVALID_ARGUMENT;
}

// DATA: mach exports the page size as a variable, not a function.
uintptr_t vm_page_size = 4096;

__attribute__((constructor)) static void vm_page_size_init(void) {
  vm_page_size = (uintptr_t)sysconf(_SC_PAGESIZE);
}

void voucher_mach_msg_set() {
  // TODO
  panic("voucher_mach_msg_set");
}
