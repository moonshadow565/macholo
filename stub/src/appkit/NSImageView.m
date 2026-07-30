#include "NSImageView.h"
#include "NSImage.h"
#include <common.h>

@implementation NSImageView {
  NSImage *_image;
  NSImageScaling _scaling;
  NSImageAlignment _alignment;
  NSImageFrameStyle _frameStyle;
  BOOL _editable;
  BOOL _animates;
}

+ (instancetype)imageViewWithImage:(NSImage *)image {
  NSImageView *view = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
  [view setImage:image];
  return view;
}

- (NSImage *)image {
  return _image;
}
- (void)setImage:(NSImage *)value {
  _image = [value retain];
}
- (NSImageScaling)imageScaling {
  return _scaling;
}
- (void)setImageScaling:(NSImageScaling)value {
  _scaling = value;
}
- (NSImageAlignment)imageAlignment {
  return _alignment;
}
- (void)setImageAlignment:(NSImageAlignment)value {
  _alignment = value;
}
- (NSImageFrameStyle)imageFrameStyle {
  return _frameStyle;
}
- (void)setImageFrameStyle:(NSImageFrameStyle)value {
  _frameStyle = value;
}
- (BOOL)isEditable {
  return _editable;
}
- (void)setEditable:(BOOL)flag {
  _editable = flag;
}
- (BOOL)animates {
  return _animates;
}
- (void)setAnimates:(BOOL)flag {
  _animates = flag;
}
@end
