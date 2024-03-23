#pragma once
#include "defines.h"

@interface NSImage : NSObject <NSCopying>
+ (NSImage *)imageNamed:(NSImageName)name;
- (instancetype)initWithSize:(NSSize)size;
- (NSSize)size;
- (void)setSize:(NSSize)value;
- (void)addRepresentation:(NSImageRep *)representation;
- (NSImageRep *)_getImageRepresentation;
@end

@interface NSImageRep : NSObject <NSCopying>
- (instancetype)init;
@end

@interface NSBitmapImageRep : NSImageRep
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
                            bitsPerPixel:(NSInteger)pBits;
@end
