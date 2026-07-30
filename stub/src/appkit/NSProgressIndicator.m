#include "NSProgressIndicator.h"
#include <common.h>

@implementation NSProgressIndicator {
  double _value;
  double _min;
  double _max;
  BOOL _indeterminate;
  BOOL _threadedAnimation;
  BOOL _displayedWhenStopped;
  NSProgressIndicatorStyle _style;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  _min = 0.0;
  _max = 100.0;
  return self;
}

- (double)doubleValue {
  return _value;
}
- (void)setDoubleValue:(double)value {
  _value = value;
}
- (double)minValue {
  return _min;
}
- (void)setMinValue:(double)value {
  _min = value;
}
- (double)maxValue {
  return _max;
}
- (void)setMaxValue:(double)value {
  _max = value;
}

- (BOOL)isIndeterminate {
  return _indeterminate;
}
- (void)setIndeterminate:(BOOL)flag {
  _indeterminate = flag;
}
- (BOOL)usesThreadedAnimation {
  return _threadedAnimation;
}
- (void)setUsesThreadedAnimation:(BOOL)flag {
  _threadedAnimation = flag;
}
- (NSProgressIndicatorStyle)style {
  return _style;
}
- (void)setStyle:(NSProgressIndicatorStyle)value {
  _style = value;
}
- (BOOL)isDisplayedWhenStopped {
  return _displayedWhenStopped;
}
- (void)setDisplayedWhenStopped:(BOOL)flag {
  _displayedWhenStopped = flag;
}

- (void)incrementBy:(double)delta {
  _value += delta;
}
- (void)startAnimation:(id)sender {
}
- (void)stopAnimation:(id)sender {
}
- (void)sizeToFit {
}
@end
