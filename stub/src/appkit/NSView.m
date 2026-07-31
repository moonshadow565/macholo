#include "NSView.h"
#include "NSTextInputContext.h"
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_video.h>
#include "Foundation/NSArray.h"
#include "GNUstepBase/GSObjCRuntime.h"
#include "NSWindow.h"
#include <common.h>

@implementation NSView {
  NSRect _frame;
  NSRect _bounds;
  NSWindow *_window;
  NSMutableArray<NSView *> *_subViews;
  BOOL _needsDisplay;
  BOOL _wantsBestResolutionOpenGLSurface;
  NSTextInputContext* _inputContext;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  _frame = frameRect;
  _bounds = (NSRect){{0.0, 0.0}, frameRect.size};
  _window = Nil;
  _subViews = [[NSMutableArray alloc] init];
   _needsDisplay = NO;
  return self;
}

- (NSWindow *)window {
  return _window;
}
- (void)setWindow:(NSWindow *)window {
  _window = window;
}

- (NSView *)superview {
  panic("Unimplemented!");
}
- (void)removeFromSuperview {
  panic("Unimplemented!");
}

- (NSArray<__kindof NSView *> *)subviews {
  return _subViews;
}
- (void)addSubview:(NSView *)view {
  [_subViews addObject:view];
}

- (NSRect)bounds {
  return _bounds;
}
- (void)setBoundsSize:(NSSize)size {
  _bounds.size = size;
  // TODO: post NSViewFrameDidChangeNotification 
}

- (BOOL)needsDisplay {
  return _needsDisplay;
}
- (void)setNeedsDisplay:(BOOL)flag {
  _needsDisplay = flag;
}
- (void)display {
  panic("Unimplemented!");
}

- (BOOL)wantsBestResolutionOpenGLSurface {
  return _wantsBestResolutionOpenGLSurface;
}
- (void)setWantsBestResolutionOpenGLSurface:(BOOL)value {
  _wantsBestResolutionOpenGLSurface = value;
}

- (BOOL)isHidden {
  panic("Unimplemented!");
}
- (void)setHidden:(BOOL)flag {
  panic("Unimplemented!");
}

- (void)drawRect:(NSRect)dirtyRect {
  panic("Unimplemented!");
}

- (NSTextInputContext *)inputContext {
  // FIXME: do actual checking here
  // if (![self conformsToProtocol: @protocol(NSTextInputClient)]) return nil;
  if (_inputContext == Nil) {
    _inputContext = [[NSTextInputContext alloc] initWithClient: self];
  }
  return _inputContext;
}

- (void)addTrackingArea:(id)trackingArea {
}
- (void)removeTrackingArea:(id)trackingArea {
}

- (void)lockFocus {
  panic("Unimplemented!");
}
- (void)unlockFocus {
  panic("Unimplemented!");
}

@end
