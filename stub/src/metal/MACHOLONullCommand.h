#pragma once
#import "MACHOLONullObject.h"

// Command encoders. Every draw/state call is an accept-everything no-op, which the
// generic fallback already provides; only the few methods whose return value is read
// need to exist by hand.
@interface MACHOLONullEncoder : MACHOLONullObject
- (instancetype)initWithCommandBuffer:(id)commandBuffer;
- (void)endEncoding;
- (id)device;
@end

@interface MACHOLONullRenderCommandEncoder : MACHOLONullEncoder
@end
@interface MACHOLONullBlitCommandEncoder : MACHOLONullEncoder
@end
@interface MACHOLONullComputeCommandEncoder : MACHOLONullEncoder
@end
@interface MACHOLONullParallelRenderCommandEncoder : MACHOLONullEncoder
@end

// The load-bearing object of the whole null renderer.
//
// A Metal engine paces itself on completion handlers: it takes a dispatch_semaphore
// before encoding a frame and signals it from -addCompletedHandler:. libdispatch here is
// the real one, so if a committed buffer never completes, the engine blocks forever on
// the second or third frame and the process just hangs. So -commit completes the buffer
// synchronously, in-line, and fires every handler before it returns.
@interface MACHOLONullCommandBuffer : MACHOLONullObject
- (instancetype)initWithQueue:(id)queue device:(id)device;
- (void)commit;
- (void)waitUntilScheduled;
- (void)waitUntilCompleted;
@end

@interface MACHOLONullCommandQueue : MACHOLONullObject
- (instancetype)initWithDevice:(id)device;
- (id)commandBuffer;
@end
