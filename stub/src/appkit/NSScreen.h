#pragma once
#include "defines.h"

extern NSDeviceDescriptionKey NSDeviceBitsPerSample;
extern NSDeviceDescriptionKey NSDeviceColorSpaceName;
extern NSDeviceDescriptionKey NSDeviceIsScreen;
extern NSDeviceDescriptionKey NSDeviceIsPrinter;
extern NSDeviceDescriptionKey NSDeviceResolution;
extern NSDeviceDescriptionKey NSDeviceSize;
extern NSDeviceDescriptionKey NSScreenNumber;

@interface NSScreen : NSObject
+ (NSScreen *)mainScreen;
+ (NSArray<NSScreen *> *)screens;

- (NSDictionary<NSDeviceDescriptionKey, id> *)deviceDescription;
- (NSRect)visibleFrame;
- (NSRect)frame;
@end
