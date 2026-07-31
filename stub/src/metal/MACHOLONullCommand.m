#import "MACHOLONullCommand.h"
#import "MTLDescriptors.h"
#import <Foundation/NSArray.h>
#include <common.h>
#include <time.h>

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

@implementation MACHOLONullEncoder {
  id _commandBuffer;  // weak: the buffer owns the encoding scope
}

- (instancetype)initWithCommandBuffer:(id)commandBuffer {
  if ((self = [super init])) _commandBuffer = commandBuffer;
  return self;
}

- (void)endEncoding {
}
- (id)device {
  return [_commandBuffer device];
}
- (id)commandBuffer {
  return _commandBuffer;
}
- (void)pushDebugGroup:(NSString *)string {
  (void)string;
}
- (void)popDebugGroup {
}
- (void)insertDebugSignpost:(NSString *)string {
  (void)string;
}
@end

@implementation MACHOLONullRenderCommandEncoder
@end
@implementation MACHOLONullBlitCommandEncoder
@end
@implementation MACHOLONullComputeCommandEncoder
@end

@implementation MACHOLONullParallelRenderCommandEncoder
// A parallel encoder vends sub-encoders that share the same command buffer.
- (id)renderCommandEncoder {
  return [[[MACHOLONullRenderCommandEncoder alloc] initWithCommandBuffer:[self commandBuffer]]
      autorelease];
}
@end

@implementation MACHOLONullCommandBuffer {
  id _queue;   // weak
  id _device;  // weak
  NSMutableArray *_scheduledHandlers;
  NSMutableArray *_completedHandlers;
  MTLCommandBufferStatus _status;
  double _gpuStart;
  double _gpuEnd;
}

- (instancetype)initWithQueue:(id)queue device:(id)device {
  if ((self = [super init])) {
    _queue = queue;
    _device = device;
    _status = MTLCommandBufferStatusNotEnqueued;
  }
  return self;
}

- (id)commandQueue {
  return _queue;
}
- (id)device {
  return _device;
}
- (id)error {
  return nil;
}
- (MTLCommandBufferStatus)status {
  return _status;
}
- (BOOL)retainedReferences {
  return YES;
}
- (double)GPUStartTime {
  return _gpuStart;
}
- (double)GPUEndTime {
  return _gpuEnd;
}
- (double)kernelStartTime {
  return _gpuStart;
}
- (double)kernelEndTime {
  return _gpuEnd;
}

- (void)enqueue {
  if (_status == MTLCommandBufferStatusNotEnqueued) _status = MTLCommandBufferStatusEnqueued;
}

// A handler registered after completion runs immediately, exactly as on a real device.
- (void)addScheduledHandler:(MACHOLOCommandBufferHandler)block {
  if (!block) return;
  if (_status >= MTLCommandBufferStatusScheduled) {
    block(self);
    return;
  }
  if (_scheduledHandlers == nil) _scheduledHandlers = [[NSMutableArray alloc] init];
  [_scheduledHandlers addObject:[[block copy] autorelease]];
}

- (void)addCompletedHandler:(MACHOLOCommandBufferHandler)block {
  if (!block) return;
  if (_status >= MTLCommandBufferStatusCompleted) {
    block(self);
    return;
  }
  if (_completedHandlers == nil) _completedHandlers = [[NSMutableArray alloc] init];
  [_completedHandlers addObject:[[block copy] autorelease]];
}

- (void)fire:(NSMutableArray **)handlers {
  NSMutableArray *list = *handlers;
  *handlers = nil;
  for (id block in list) ((MACHOLOCommandBufferHandler)block)(self);
  [list release];
}

- (void)commit {
  if (_status >= MTLCommandBufferStatusCommitted) return;
  _gpuStart = now_seconds();
  _status = MTLCommandBufferStatusScheduled;
  [self fire:&_scheduledHandlers];
  _gpuEnd = now_seconds();
  _status = MTLCommandBufferStatusCompleted;
  [self fire:&_completedHandlers];
}

- (void)waitUntilScheduled {
}
- (void)waitUntilCompleted {
}

- (id)renderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor *)descriptor {
  (void)descriptor;
  return [[[MACHOLONullRenderCommandEncoder alloc] initWithCommandBuffer:self] autorelease];
}
- (id)parallelRenderCommandEncoderWithDescriptor:(MTLRenderPassDescriptor *)descriptor {
  (void)descriptor;
  return
      [[[MACHOLONullParallelRenderCommandEncoder alloc] initWithCommandBuffer:self] autorelease];
}
- (id)blitCommandEncoder {
  return [[[MACHOLONullBlitCommandEncoder alloc] initWithCommandBuffer:self] autorelease];
}
- (id)computeCommandEncoder {
  return [[[MACHOLONullComputeCommandEncoder alloc] initWithCommandBuffer:self] autorelease];
}

- (void)presentDrawable:(id)drawable {
  (void)drawable;
}
- (void)presentDrawable:(id)drawable atTime:(double)time {
  (void)drawable;
  (void)time;
}
- (void)presentDrawable:(id)drawable afterMinimumDuration:(double)duration {
  (void)drawable;
  (void)duration;
}

- (void)pushDebugGroup:(NSString *)string {
  (void)string;
}
- (void)popDebugGroup {
}

- (void)dealloc {
  [_scheduledHandlers release];
  [_completedHandlers release];
  [super dealloc];
}
@end

@implementation MACHOLONullCommandQueue {
  id _device;  // weak: the device owns every queue
}

- (instancetype)initWithDevice:(id)device {
  if ((self = [super init])) _device = device;
  return self;
}

- (id)device {
  return _device;
}

- (id)commandBuffer {
  return
      [[[MACHOLONullCommandBuffer alloc] initWithQueue:self device:_device] autorelease];
}

- (id)commandBufferWithUnretainedReferences {
  return [self commandBuffer];
}

- (void)insertDebugCaptureBoundary {
}
@end
