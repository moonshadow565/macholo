#import "MACHOLONullDevice.h"
#import "MACHOLONullCommand.h"
#import "MACHOLONullResources.h"
#import <Foundation/NSArray.h>
#include <common.h>

@implementation MACHOLONullFunction {
  NSString *_name;
  id _device;  // weak
}

- (instancetype)initWithName:(NSString *)name device:(id)device {
  if ((self = [super init])) {
    _name = [name copy];
    _device = device;
  }
  return self;
}

- (NSString *)name {
  return _name;
}
- (id)device {
  return _device;
}
// Every shader in the corpus is a single vertex or fragment `wrapped_main`; nothing asks
// us to tell them apart, and we could not without parsing AIR.
- (NSUInteger)functionType {
  return 0;
}
- (id)vertexAttributes {
  return nil;
}
- (id)stageInputAttributes {
  return nil;
}

- (void)dealloc {
  [_name release];
  [super dealloc];
}
@end

@implementation MACHOLONullLibrary {
  id _device;  // weak
}

- (instancetype)initWithDevice:(id)device {
  if ((self = [super init])) _device = device;
  return self;
}

- (id)device {
  return _device;
}

// +1 retained, per the "new" naming convention the caller compiles against.
- (id)newFunctionWithName:(NSString *)name {
  return [[MACHOLONullFunction alloc] initWithName:name device:_device];
}

- (id)newFunctionWithName:(NSString *)name constantValues:(id)values error:(NSError **)error {
  (void)values;
  if (error) *error = nil;
  return [self newFunctionWithName:name];
}

- (NSArray<NSString *> *)functionNames {
  return @[ @"wrapped_main" ];
}
@end

@implementation MACHOLONullPipelineState {
  id _device;  // weak
}

- (instancetype)initWithDevice:(id)device {
  if ((self = [super init])) _device = device;
  return self;
}

- (id)device {
  return _device;
}
- (NSUInteger)maxTotalThreadsPerThreadgroup {
  return 1024;
}
- (NSUInteger)threadExecutionWidth {
  return 32;
}
- (NSUInteger)staticThreadgroupMemoryLength {
  return 0;
}
@end

@implementation MACHOLONullDevice

- (NSString *)name {
  return @"macholo null";
}
- (uint64_t)registryID {
  return 0x6d6163686f6c6fULL;  // "macholo"
}
- (uint64_t)peerGroupID {
  return 0;
}
- (BOOL)isLowPower {
  return NO;
}
- (BOOL)isHeadless {
  return YES;
}
- (BOOL)isRemovable {
  return NO;
}
- (BOOL)hasUnifiedMemory {
  return YES;
}
- (uint64_t)recommendedMaxWorkingSetSize {
  return 2ULL * 1024 * 1024 * 1024;
}
- (uint64_t)maxBufferLength {
  return 1ULL * 1024 * 1024 * 1024;
}
- (uint64_t)currentAllocatedSize {
  return 0;
}
- (MTLSize)maxThreadsPerThreadgroup {
  return MTLSizeMake(1024, 1024, 64);
}
- (NSUInteger)maxThreadgroupMemoryLength {
  return 32 * 1024;
}

// Say yes to everything: the engine uses these to pick a feature path, and any path it
// picks encodes into no-op encoders anyway.
- (BOOL)supportsFeatureSet:(NSUInteger)featureSet {
  (void)featureSet;
  return YES;
}
- (BOOL)supportsFamily:(NSInteger)family {
  (void)family;
  return YES;
}
- (BOOL)supportsTextureSampleCount:(NSUInteger)sampleCount {
  (void)sampleCount;
  return YES;
}
- (BOOL)isDepth24Stencil8PixelFormatSupported {
  return YES;
}
- (BOOL)supportsShaderBarycentricCoordinates {
  return YES;
}
- (BOOL)areProgrammableSamplePositionsSupported {
  return YES;
}
- (BOOL)areRasterOrderGroupsSupported {
  return YES;
}
- (NSUInteger)argumentBuffersSupport {
  return 1;
}
- (NSUInteger)readWriteTextureSupport {
  return 2;
}

- (id)newCommandQueue {
  return [[MACHOLONullCommandQueue alloc] initWithDevice:self];
}
- (id)newCommandQueueWithMaxCommandBufferCount:(NSUInteger)count {
  (void)count;
  return [self newCommandQueue];
}

- (id)newLibraryWithData:(id)data error:(NSError **)error {
  (void)data;
  if (error) *error = nil;
  return [[MACHOLONullLibrary alloc] initWithDevice:self];
}
- (id)newLibraryWithFile:(NSString *)filepath error:(NSError **)error {
  (void)filepath;
  if (error) *error = nil;
  return [[MACHOLONullLibrary alloc] initWithDevice:self];
}
- (id)newLibraryWithURL:(id)url error:(NSError **)error {
  (void)url;
  if (error) *error = nil;
  return [[MACHOLONullLibrary alloc] initWithDevice:self];
}
- (id)newLibraryWithSource:(NSString *)source options:(id)options error:(NSError **)error {
  (void)source;
  (void)options;
  if (error) *error = nil;
  return [[MACHOLONullLibrary alloc] initWithDevice:self];
}
- (id)newDefaultLibrary {
  return [[MACHOLONullLibrary alloc] initWithDevice:self];
}

// Pipeline creation never fails; a failure here is the one thing an engine is guaranteed
// to treat as fatal.
- (id)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                                     error:(NSError **)error {
  (void)descriptor;
  if (error) *error = nil;
  return [[MACHOLONullPipelineState alloc] initWithDevice:self];
}
- (id)newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor *)descriptor
                                   options:(NSUInteger)options
                                reflection:(id *)reflection
                                     error:(NSError **)error {
  (void)descriptor;
  (void)options;
  if (reflection) *reflection = nil;
  if (error) *error = nil;
  return [[MACHOLONullPipelineState alloc] initWithDevice:self];
}
- (id)newComputePipelineStateWithFunction:(id)function error:(NSError **)error {
  (void)function;
  if (error) *error = nil;
  return [[MACHOLONullPipelineState alloc] initWithDevice:self];
}

- (id)newDepthStencilStateWithDescriptor:(MTLDepthStencilDescriptor *)descriptor {
  (void)descriptor;
  return [[MACHOLONullPipelineState alloc] initWithDevice:self];
}
- (id)newSamplerStateWithDescriptor:(MTLSamplerDescriptor *)descriptor {
  (void)descriptor;
  return [[MACHOLONullPipelineState alloc] initWithDevice:self];
}

- (id)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options {
  (void)options;
  return [[MACHOLONullBuffer alloc] initWithLength:length device:self];
}
- (id)newBufferWithBytes:(const void *)pointer
                  length:(NSUInteger)length
                 options:(MTLResourceOptions)options {
  (void)options;
  return [[MACHOLONullBuffer alloc] initWithBytes:pointer length:length device:self];
}
- (id)newBufferWithBytesNoCopy:(void *)pointer
                        length:(NSUInteger)length
                       options:(MTLResourceOptions)options
                   deallocator:(void (^)(void *, NSUInteger))deallocator {
  (void)deallocator;  // we copy, so the caller's block must never run
  return [[MACHOLONullBuffer alloc] initWithBytes:pointer length:length device:self];
}

- (id)newTextureWithDescriptor:(MTLTextureDescriptor *)descriptor {
  return [[MACHOLONullTexture alloc] initWithDescriptor:descriptor device:self];
}

- (id)newFence {
  return [[MACHOLONullObject alloc] init];
}
- (id)newEvent {
  return [[MACHOLONullObject alloc] init];
}
- (id)newSharedEvent {
  return [[MACHOLONullObject alloc] init];
}
@end

id macholo_metal_null_device(void) {
  static MACHOLONullDevice *device;
  if (device == nil) device = [[MACHOLONullDevice alloc] init];
  return device;
}
