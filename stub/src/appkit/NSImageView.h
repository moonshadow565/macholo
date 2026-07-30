#pragma once
#include "NSView.h"

typedef NSUInteger NSImageScaling;
typedef NSUInteger NSImageAlignment;
typedef NSUInteger NSImageFrameStyle;

// Splash-screen only: holds the splash NSImage, never draws.
@interface NSImageView : NSView
+ (instancetype)imageViewWithImage:(NSImage *)image;

- (NSImage *)image;
- (void)setImage:(NSImage *)value;
- (NSImageScaling)imageScaling;
- (void)setImageScaling:(NSImageScaling)value;
- (NSImageAlignment)imageAlignment;
- (void)setImageAlignment:(NSImageAlignment)value;
- (NSImageFrameStyle)imageFrameStyle;
- (void)setImageFrameStyle:(NSImageFrameStyle)value;
- (BOOL)isEditable;
- (void)setEditable:(BOOL)flag;
- (BOOL)animates;
- (void)setAnimates:(BOOL)flag;
@end
