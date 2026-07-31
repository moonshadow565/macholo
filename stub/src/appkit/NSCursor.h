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

// The standard system cursors. SDL owns the real pointer shape, so these are
// distinguishable instances that all draw as the default arrow.
+ (NSCursor *)arrowCursor;
+ (NSCursor *)IBeamCursor;
+ (NSCursor *)pointingHandCursor;
+ (NSCursor *)closedHandCursor;
+ (NSCursor *)openHandCursor;
+ (NSCursor *)crosshairCursor;
+ (NSCursor *)resizeLeftRightCursor;
+ (NSCursor *)resizeUpDownCursor;
+ (NSCursor *)operationNotAllowedCursor;
@end
