#pragma once
#import "MACHOLONullObject.h"
#import "MTLDescriptors.h"

// Opaque success objects. Nothing in a metallib is parsed: a library accepts any data and
// vends a function for any name, and pipeline creation never fails. See docs/METAL-NULL.md.
@interface MACHOLONullFunction : MACHOLONullObject
- (instancetype)initWithName:(NSString *)name device:(id)device;
- (NSString *)name;
@end

@interface MACHOLONullLibrary : MACHOLONullObject
- (instancetype)initWithDevice:(id)device;
- (id)newFunctionWithName:(NSString *)name;
@end

@interface MACHOLONullPipelineState : MACHOLONullObject
- (instancetype)initWithDevice:(id)device;
@end

@interface MACHOLONullDevice : MACHOLONullObject
@end

// The single device instance, created on first use. Never nil.
id macholo_metal_null_device(void);
