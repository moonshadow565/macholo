#pragma once
#include "NSResponder.h"
#include "defines.h"

@interface NSView : NSResponder
- (instancetype)initWithFrame:(NSRect)frameRect;

- (NSWindow *)window;
- (void) setWindow: (NSWindow*)window; // fake

- (NSView *)superview;
- (void)removeFromSuperview;

- (NSArray<__kindof NSView *> *)subviews;
- (void)addSubview:(NSView *)view;

- (NSRect)bounds;
- (void)setBoundsSize:(NSSize)size;

- (BOOL)isHidden;
- (void)setHidden:(BOOL)flag;

- (void)drawRect:(NSRect)dirtyRect;

- (BOOL)needsDisplay;
- (void)setNeedsDisplay:(BOOL)flag;
- (void)display;

- (NSTextInputContext *)inputContext;

// Layer backing. There is no CoreAnimation here; the only layer 16.15 ever installs is a
// CAMetalLayer (stub/src/metal/), which the view just has to hold on to and hand back.
- (BOOL)wantsLayer;
- (void)setWantsLayer:(BOOL)flag;
- (id)layer;
- (void)setLayer:(id)layer;

// NSTrackingArea itself is not implemented (16.15 leaves the class ref unresolved, so the
// game hands us nil); mouse enter/exit/move tracking is unconditional in NSEvent anyway.
- (void)addTrackingArea:(id)trackingArea;
- (void)removeTrackingArea:(id)trackingArea;

- (void)lockFocus;
- (void)unlockFocus;

- (BOOL)wantsBestResolutionOpenGLSurface;
- (void)setWantsBestResolutionOpenGLSurface:(BOOL)value;
@end
