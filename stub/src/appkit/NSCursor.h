#pragma once
#include "defines.h"

@interface NSCursor : NSObject
- (id)initWithImage:(NSImage *)image hotSpot:(NSPoint)hotSpot;

- (NSImage *)image;
- (NSPoint)hotSpot;
+ (void)hide;
+ (void)unhide;
+ (void)setHiddenUntilMouseMoves:(BOOL)flag;

+ (void)pop;
- (void)pop;
- (void)push;
- (void)set;

+ (NSCursor *)currentCursor;
@end
