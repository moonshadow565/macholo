#pragma once
#include "NSView.h"

typedef NSUInteger NSProgressIndicatorStyle;

enum {
  NSProgressIndicatorStyleBar = 0,
  NSProgressIndicatorStyleSpinning = 1,
};

// Splash-screen only: the client creates one, spins it, and throws it away. Nothing is
// ever drawn, so the whole class is property bookkeeping.
@interface NSProgressIndicator : NSView
- (double)doubleValue;
- (void)setDoubleValue:(double)value;
- (double)minValue;
- (void)setMinValue:(double)value;
- (double)maxValue;
- (void)setMaxValue:(double)value;

- (BOOL)isIndeterminate;
- (void)setIndeterminate:(BOOL)flag;
- (BOOL)usesThreadedAnimation;
- (void)setUsesThreadedAnimation:(BOOL)flag;
- (NSProgressIndicatorStyle)style;
- (void)setStyle:(NSProgressIndicatorStyle)value;
- (BOOL)isDisplayedWhenStopped;
- (void)setDisplayedWhenStopped:(BOOL)flag;

- (void)incrementBy:(double)delta;
- (void)startAnimation:(id)sender;
- (void)stopAnimation:(id)sender;
- (void)sizeToFit;
@end
