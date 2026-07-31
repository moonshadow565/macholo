#import "MACHOLONullResources.h"
#include <stdlib.h>
#include <string.h>
#include <common.h>

@implementation MACHOLONullBuffer {
  void *_contents;
  NSUInteger _length;
  id _device;  // weak on purpose: the device outlives every resource
}

- (instancetype)initWithLength:(NSUInteger)length device:(id)device {
  if ((self = [super init])) {
    _length = length;
    _device = device;
    // calloc, not malloc: engines routinely read a buffer region they have not written
    // yet (padding, unused instance slots), and deterministic zeros beat heap garbage.
    _contents = calloc(1, length ? length : 1);
    assert_that(_contents != NULL);
  }
  return self;
}

- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length device:(id)device {
  if ((self = [self initWithLength:length device:device]) && bytes && length)
    memcpy(_contents, bytes, length);
  return self;
}

- (void *)contents {
  return _contents;
}
- (NSUInteger)length {
  return _length;
}
- (NSUInteger)allocatedSize {
  return _length;
}
- (id)device {
  return _device;
}

// Shared-memory bookkeeping: nothing to flush to, nothing to evict.
- (void)didModifyRange:(NSRange)range {
  (void)range;
}
- (NSUInteger)setPurgeableState:(MTLPurgeableState)state {
  return state;
}
- (void)addDebugMarker:(NSString *)marker range:(NSRange)range {
  (void)marker;
  (void)range;
}
- (void)removeAllDebugMarkers {
}

- (void)dealloc {
  free(_contents);
  [super dealloc];
}
@end

@implementation MACHOLONullTexture {
  id _device;  // weak, see above
}
@synthesize textureType = _textureType;
@synthesize pixelFormat = _pixelFormat;
@synthesize width = _width;
@synthesize height = _height;
@synthesize depth = _depth;
@synthesize mipmapLevelCount = _mipmapLevelCount;
@synthesize sampleCount = _sampleCount;
@synthesize arrayLength = _arrayLength;
@synthesize usage = _usage;
@synthesize storageMode = _storageMode;
@synthesize cpuCacheMode = _cpuCacheMode;

- (instancetype)initWithDescriptor:(MTLTextureDescriptor *)descriptor device:(id)device {
  if ((self = [super init])) {
    _device = device;
    _textureType = descriptor.textureType;
    _pixelFormat = descriptor.pixelFormat;
    _width = descriptor.width;
    _height = descriptor.height;
    _depth = descriptor.depth;
    _mipmapLevelCount = descriptor.mipmapLevelCount;
    _sampleCount = descriptor.sampleCount;
    _arrayLength = descriptor.arrayLength;
    _usage = descriptor.usage;
    _storageMode = descriptor.storageMode;
    _cpuCacheMode = descriptor.cpuCacheMode;
  }
  return self;
}

- (instancetype)initWithWidth:(NSUInteger)width
                       height:(NSUInteger)height
                  pixelFormat:(MTLPixelFormat)pixelFormat
                       device:(id)device {
  if ((self = [super init])) {
    _device = device;
    _textureType = MTLTextureType2D;
    _pixelFormat = pixelFormat;
    _width = width;
    _height = height;
    _depth = 1;
    _mipmapLevelCount = 1;
    _sampleCount = 1;
    _arrayLength = 1;
  }
  return self;
}

- (id)device {
  return _device;
}
- (id)parentTexture {
  return nil;
}
- (id)buffer {
  return nil;
}
- (id)rootResource {
  return self;
}
- (BOOL)isFramebufferOnly {
  return NO;
}
- (BOOL)framebufferOnly {
  return NO;
}

// Uploads go nowhere.
- (void)replaceRegion:(MTLRegion)region
          mipmapLevel:(NSUInteger)level
            withBytes:(const void *)bytes
          bytesPerRow:(NSUInteger)bytesPerRow {
  (void)region;
  (void)level;
  (void)bytes;
  (void)bytesPerRow;
}

- (void)replaceRegion:(MTLRegion)region
          mipmapLevel:(NSUInteger)level
                slice:(NSUInteger)slice
            withBytes:(const void *)bytes
          bytesPerRow:(NSUInteger)bytesPerRow
        bytesPerImage:(NSUInteger)bytesPerImage {
  (void)region;
  (void)level;
  (void)slice;
  (void)bytes;
  (void)bytesPerRow;
  (void)bytesPerImage;
}

// Readback yields a deterministic black image rather than uninitialised caller memory.
- (void)getBytes:(void *)bytes
     bytesPerRow:(NSUInteger)bytesPerRow
      fromRegion:(MTLRegion)region
     mipmapLevel:(NSUInteger)level {
  (void)level;
  if (bytes && bytesPerRow && region.size.height)
    memset(bytes, 0, bytesPerRow * region.size.height);
}

- (void)getBytes:(void *)bytes
      bytesPerRow:(NSUInteger)bytesPerRow
    bytesPerImage:(NSUInteger)bytesPerImage
       fromRegion:(MTLRegion)region
      mipmapLevel:(NSUInteger)level
            slice:(NSUInteger)slice {
  (void)level;
  (void)slice;
  if (!bytes) return;
  NSUInteger n = bytesPerImage ? bytesPerImage * (region.size.depth ? region.size.depth : 1)
                               : bytesPerRow * region.size.height;
  if (n) memset(bytes, 0, n);
}

- (id)newTextureViewWithPixelFormat:(MTLPixelFormat)pixelFormat {
  MACHOLONullTexture *view = [[MACHOLONullTexture alloc] initWithWidth:_width
                                                                height:_height
                                                           pixelFormat:pixelFormat
                                                                device:_device];
  return view;
}

- (NSUInteger)setPurgeableState:(MTLPurgeableState)state {
  return state;
}
@end
