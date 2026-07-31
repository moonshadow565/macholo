#import "CAMetalLayer.h"
#import "MACHOLONullResources.h"
#include <common.h>
#include <time.h>

@implementation MACHOLONullDrawable {
  id _layer;  // weak: the layer owns the drawable's lifetime in practice
  id _texture;
  double _presentedTime;
}

- (instancetype)initWithLayer:(id)layer texture:(id)texture {
  if ((self = [super init])) {
    _layer = layer;
    _texture = [texture retain];
  }
  return self;
}

- (id)texture {
  return _texture;
}
- (id)layer {
  return _layer;
}
- (NSUInteger)drawableID {
  return 0;
}

- (void)present {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  _presentedTime = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
- (void)presentAtTime:(double)presentationTime {
  _presentedTime = presentationTime;
}
- (void)presentAfterMinimumDuration:(double)duration {
  (void)duration;
  [self present];
}
- (double)presentedTime {
  return _presentedTime;
}
- (void)addPresentedHandler:(void (^)(id))block {
  if (block) block(self);
}

- (void)dealloc {
  [_texture release];
  [super dealloc];
}
@end

@implementation CAMetalLayer
@synthesize device = _device;
@synthesize pixelFormat = _pixelFormat;
@synthesize framebufferOnly = _framebufferOnly;
@synthesize drawableSize = _drawableSize;
@synthesize contentsScale = _contentsScale;
@synthesize frame = _frame;
@synthesize bounds = _bounds;

+ (instancetype)layer {
  return [[[self alloc] init] autorelease];
}

- (instancetype)init {
  if ((self = [super init])) {
    _pixelFormat = MTLPixelFormatBGRA8Unorm;
    _framebufferOnly = YES;
    _contentsScale = 1.0;
    _drawableSize = NSMakeSize(1280, 800);
  }
  return self;
}

- (void)setFrame:(NSRect)frame {
  _frame = frame;
  _bounds = NSMakeRect(0, 0, frame.size.width, frame.size.height);
}

- (id)nextDrawable {
  NSUInteger w = (NSUInteger)_drawableSize.width;
  NSUInteger h = (NSUInteger)_drawableSize.height;
  if (w == 0) w = 1;
  if (h == 0) h = 1;

  MACHOLONullTexture *texture = [[MACHOLONullTexture alloc] initWithWidth:w
                                                                  height:h
                                                             pixelFormat:_pixelFormat
                                                                  device:_device];
  MACHOLONullDrawable *drawable = [[MACHOLONullDrawable alloc] initWithLayer:self
                                                                     texture:texture];
  [texture release];
  return [drawable autorelease];
}

// CALayer surface the engine reaches for while wiring the layer into a view.
- (void)setNeedsDisplay {
}
- (void)setNeedsLayout {
}
- (void)addSublayer:(id)layer {
  (void)layer;
}
- (void)removeFromSuperlayer {
}
- (id)superlayer {
  return nil;
}
- (id)sublayers {
  return nil;
}

- (void)dealloc {
  [_device release];
  [super dealloc];
}
@end
