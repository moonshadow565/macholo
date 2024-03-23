#include "NSImage.h"
#include <common.h>

@implementation NSImage {
  NSSize _size;
  NSImageRep* _imageRepresentation;
}

+ (NSImage *)imageNamed:(NSImageName)name {
  panic("Unimplemented");
}

- (id)copyWithZone:(NSZone *)zone {
  panic("Unimplemented");
}

- (instancetype)initWithSize:(NSSize)size {
  _size = size;
  return self;
}

- (NSSize)size {
  return _size;
}
- (void)setSize:(NSSize)value {
  _size = value;
}

- (void)addRepresentation:(NSImageRep *)representation {
  _imageRepresentation = representation;
}

- (NSImageRep *)_getImageRepresentation {
  return _imageRepresentation;
}

@end

@implementation NSImageRep
- (id)copyWithZone:(NSZone *)zone {
  panic("Unimplemented");
}

- (instancetype)init {
  [super init];
  return self;
}

@end

@implementation NSBitmapImageRep
- (instancetype)initWithBitmapDataPlanes:(unsigned char *_Nullable *)planes
                              pixelsWide:(NSInteger)width
                              pixelsHigh:(NSInteger)height
                           bitsPerSample:(NSInteger)bps
                         samplesPerPixel:(NSInteger)spp
                                hasAlpha:(BOOL)alpha
                                isPlanar:(BOOL)isPlanar
                          colorSpaceName:(NSColorSpaceName)colorSpaceName
                            bitmapFormat:(NSBitmapFormat)bitmapFormat
                             bytesPerRow:(NSInteger)rBytes
                            bitsPerPixel:(NSInteger)pBits {
  return self;
}
@end

