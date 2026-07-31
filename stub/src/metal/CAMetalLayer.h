#pragma once
#import "MACHOLONullObject.h"
#import <Foundation/NSGeometry.h>

// CAMetalLayer is the only QuartzCore import in 16.15, and the only place where the null
// renderer can stall the game rather than merely draw nothing: -nextDrawable on a real
// device blocks until the display pipeline frees one. Here it always returns a fresh
// drawable immediately.
//
// NSSize/NSRect stand in for CGSize/CGRect — same ABI, and pulling in the CoreGraphics
// module's header from here would be a layering inversion.

@interface MACHOLONullDrawable : MACHOLONullObject
- (instancetype)initWithLayer:(id)layer texture:(id)texture;
- (id)texture;
- (id)layer;
- (void)present;
@end

@interface CAMetalLayer : MACHOLONullObject
+ (instancetype)layer;
@property(nonatomic, retain) id device;
@property(nonatomic, assign) NSUInteger pixelFormat;
@property(nonatomic, assign) BOOL framebufferOnly;
@property(nonatomic, assign) NSSize drawableSize;
@property(nonatomic, assign) double contentsScale;
@property(nonatomic, assign) NSRect frame;
@property(nonatomic, assign) NSRect bounds;
- (id)nextDrawable;
@end
