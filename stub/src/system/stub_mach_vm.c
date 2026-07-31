// mach VM / thread-inspection surface WC3 imports. Most of it belongs to its
// anti-tamper and crash-reporting paths: walk your own memory, snapshot other
// threads, install exception ports. None of that is reproducible on Linux, so the
// rule here is "answer honestly where the answer is cheap, fail cleanly otherwise" --
// a clean KERN_FAILURE keeps the caller on its own error path instead of trapping.
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <common.h>

typedef int kern_return_t;
typedef unsigned mach_port_t;
typedef mach_port_t task_t;
typedef mach_port_t thread_t;
typedef uint64_t mach_vm_address_t;
typedef uint64_t mach_vm_size_t;
typedef int vm_prot_t;
typedef unsigned vm_inherit_t;
typedef unsigned mach_msg_type_number_t;

#define KERN_FAILURE 5
#define KERN_INVALID_ADDRESS 1
#define KERN_NO_ACCESS 8

// ---------------------------------------------------------------------------
// VM
// ---------------------------------------------------------------------------

// NOTE: mach_vm_remap lives in the loader (src/lib.cpp): a real alias needs the
// image's backing file descriptor, which only the loader has.

// "What is mapped at this address?" There is no cheap honest answer without parsing
// /proc/self/maps, and every caller here treats failure as "nothing there".
kern_return_t mach_vm_region(task_t target_task, mach_vm_address_t *address, mach_vm_size_t *size, int flavor,
                             void *info, mach_msg_type_number_t *info_count, mach_port_t *object_name) {
  return KERN_INVALID_ADDRESS;
}

kern_return_t vm_copy(task_t target_task, uintptr_t source_address, uintptr_t size, uintptr_t dest_address) {
  if (!source_address || !dest_address) return KERN_INVALID_ADDRESS;
  memcpy((void *)dest_address, (void const *)source_address, (size_t)size);
  return 0;
}

// ---------------------------------------------------------------------------
// Thread inspection -- crash reporter territory
// ---------------------------------------------------------------------------

kern_return_t task_threads(task_t target_task, thread_t **act_list, mach_msg_type_number_t *act_list_count) {
  // Report an empty thread list rather than failing: a crash reporter that gets zero
  // threads writes an empty report, one that gets an error may retry forever.
  if (act_list) *act_list = NULL;
  if (act_list_count) *act_list_count = 0;
  return 0;
}

kern_return_t thread_get_state(thread_t target, int flavor, void *state, mach_msg_type_number_t *count) {
  if (count) *count = 0;
  return KERN_FAILURE;
}

kern_return_t thread_set_state(thread_t target, int flavor, void *state, mach_msg_type_number_t count) {
  return KERN_FAILURE;
}

kern_return_t thread_info(thread_t target, int flavor, void *info, mach_msg_type_number_t *count) {
  if (count) *count = 0;
  return KERN_FAILURE;
}

kern_return_t thread_suspend(thread_t target) {
  // Suspending an arbitrary pthread has no portable equivalent; claiming success is
  // safer than an error, because callers that fail here tend to abort the process.
  return 0;
}

kern_return_t thread_resume(thread_t target) {
  return 0;
}

// ---------------------------------------------------------------------------
// Exception / MIG plumbing
// ---------------------------------------------------------------------------

kern_return_t exception_raise(mach_port_t exception_port, mach_port_t thread, mach_port_t task, int exception,
                              uintptr_t *code, mach_msg_type_number_t code_count) {
  return KERN_FAILURE;
}

kern_return_t exception_raise_state(mach_port_t exception_port, int exception, uintptr_t *code,
                                    mach_msg_type_number_t code_count, int *flavor, void *old_state,
                                    mach_msg_type_number_t old_state_count, void *new_state,
                                    mach_msg_type_number_t *new_state_count) {
  return KERN_FAILURE;
}

kern_return_t exception_raise_state_identity(mach_port_t exception_port, mach_port_t thread, mach_port_t task,
                                             int exception, uintptr_t *code, mach_msg_type_number_t code_count,
                                             int *flavor, void *old_state, mach_msg_type_number_t old_state_count,
                                             void *new_state, mach_msg_type_number_t *new_state_count) {
  return KERN_FAILURE;
}

kern_return_t mach_msg_server(void *demux, mach_msg_type_number_t max_size, mach_port_t rcv_name, unsigned options) {
  // The exception-handler thread's message pump. There are no mach messages to serve,
  // so return instead of spinning; the thread then just exits.
  return KERN_FAILURE;
}

kern_return_t mach_port_insert_right(task_t task, mach_port_t name, mach_port_t poly, unsigned polyPoly) {
  return 0;
}

kern_return_t semaphore_signal_all(unsigned semaphore) {
  // No mach semaphores exist here (see stub_kernel.c: the $darwin$semaphore_* family
  // is POSIX-backed and this entry point is never paired with them).
  return 0;
}

// MIG reply-port bookkeeping. One port per thread would be correct; nothing reads the
// value because every MIG call above fails before using it.
mach_port_t mig_get_reply_port(void) {
  return 0;
}

void mig_put_reply_port(mach_port_t reply_port) {
}

void mig_dealloc_reply_port(mach_port_t reply_port) {
}

// mach_override's entry point: overwrite a function with a jump to another. The
// callers use it for optional instrumentation, so refuse cleanly.
kern_return_t mach_override_ptr(void *target, const void *replacement, void **originalp) {
  if (originalp) *originalp = NULL;
  return KERN_FAILURE;
}
