#import <Foundation/NSData.h>

#include <common.h>

typedef void *dispatch_block_t;
typedef void *dispatch_data_t;
typedef struct dispatch_queue_s *dispatch_queue_t;

// The only dispatch_data use in any audited version: 16.15 wraps a metallib blob to hand
// to -[MTLDevice newLibraryWithData:error:], and never asks the object anything
// afterwards (dispatch_data_get_size/_apply/_release are in no version's import list).
//
// This is an ObjC file for the one reason that matters: on Darwin dispatch_data_t *is* an
// ObjC object, so the caller's ARC releases it with objc_release, which 16.15 does import.
// An NSData survives that; a malloc'd handle would not.
//
// The destructor argument is always DISPATCH_DATA_DESTRUCTOR_DEFAULT here (the FREE and
// MUNMAP destructors are imported symbols and no version imports them), and default means
// "copy the buffer" — exactly NSData's own semantics.
dispatch_data_t dispatch_data_create(const void *buffer,
                                     size_t size,
                                     dispatch_queue_t queue,
                                     dispatch_block_t destructor) {
  (void)queue;
  assert_that(destructor == NULL);
  // Returned +1, per dispatch_data_create's create-rule ownership.
  return [[NSData alloc] initWithBytes:buffer length:size];
}
