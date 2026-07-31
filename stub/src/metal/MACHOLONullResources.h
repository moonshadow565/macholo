#pragma once
#import "MACHOLONullObject.h"
#import "MTLDescriptors.h"

// Buffers are the one resource whose contents genuinely matter: the engine writes vertex
// and constant data straight through -contents and expects to read it back, so these are
// real malloc'd allocations, not stubs.
@interface MACHOLONullBuffer : MACHOLONullObject
- (instancetype)initWithLength:(NSUInteger)length device:(id)device;
- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length device:(id)device;
- (void *)contents;
- (NSUInteger)length;
- (id)device;
@end

// Textures carry their descriptor's shape so the engine's own bookkeeping stays
// consistent; the pixels are never stored. Writes are dropped, reads return zeros.
@interface MACHOLONullTexture : MACHOLONullObject
- (instancetype)initWithDescriptor:(MTLTextureDescriptor *)descriptor device:(id)device;
- (instancetype)initWithWidth:(NSUInteger)width
                       height:(NSUInteger)height
                  pixelFormat:(MTLPixelFormat)pixelFormat
                       device:(id)device;
@property(nonatomic, readonly) MTLTextureType textureType;
@property(nonatomic, readonly) MTLPixelFormat pixelFormat;
@property(nonatomic, readonly) NSUInteger width;
@property(nonatomic, readonly) NSUInteger height;
@property(nonatomic, readonly) NSUInteger depth;
@property(nonatomic, readonly) NSUInteger mipmapLevelCount;
@property(nonatomic, readonly) NSUInteger sampleCount;
@property(nonatomic, readonly) NSUInteger arrayLength;
@property(nonatomic, readonly) MTLTextureUsage usage;
@property(nonatomic, readonly) MTLStorageMode storageMode;
@property(nonatomic, readonly) MTLCPUCacheMode cpuCacheMode;
- (id)device;
@end
