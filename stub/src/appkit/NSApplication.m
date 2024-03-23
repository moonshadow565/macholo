#include "NSApplication.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include "GNUstepBase/GSObjCRuntime.h"
#include "NSEvent.h"
#include "NSWindow.h"
#include "SDL_video.h"
#include <common.h>

NSApplication *NSApp = Nil;

@implementation NSApplication {
  id<NSApplicationDelegate> _delegate;
  NSEvent *_current_event;
}

+ (NSApplication *)sharedApplication {
  // panic("Unimplemented!");
  if (NSApp == nil) {
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    NSApp = [[[NSApplication alloc] init] retain];
  }
  return NSApp;
}

- init {
  [super init];
  _current_event = [NSEvent alloc];
  return self;
}

- (id)delegate {
  return _delegate;
}

- (void)setDelegate:(id)delegate {
  if (delegate == _delegate)  // nothing to do:
    return;

  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  if (_delegate)  // remove old:
    [center removeObserver:_delegate name:nil object:self];

  _delegate = delegate;
#define REGISTER($name)                                              \
  if ([_delegate respondsToSelector:@selector(application##$name:)]) \
  [center addObserver:_delegate                                      \
             selector:@selector(application##$name:)                 \
                 name:@"NSApplication" #$name "Notification"         \
               object:self]
  REGISTER(DidBecomeActive);
  REGISTER(DidChangeOcclusionState);
  REGISTER(DidChangeScreenParameters);
  REGISTER(DidFinishLaunching);
  REGISTER(DidHide);
  REGISTER(DidResignActive);
  REGISTER(DidUnhide);
  REGISTER(DidUpdate);
  REGISTER(WillBecomeActive);
  REGISTER(WillFinishLaunching);
  REGISTER(WillHide);
  REGISTER(WillResignActive);
  REGISTER(WillTerminate);
  REGISTER(WillUnhide);
  REGISTER(WillUpdate);
#undef REGISTER
}

- (NSEvent *)currentEvent {
  panic("Unimplemented!");
}

- (NSEvent *)nextEventMatchingMask:(NSEventMask)mask
                         untilDate:(NSDate *)untilDate
                            inMode:(NSString *)mode
                           dequeue:(BOOL)dequeue {
  assert_that(dequeue == YES);
  assert_that(mask == NSEventMaskAny);
  assert_that(untilDate == NULL || [untilDate timeIntervalSinceNow] <= 0.0);

  SDL_Event sdl_event = {};
  if (!SDL_PollEvent(&sdl_event)) return nil;

  switch (sdl_event.type) {
    // Internal book keeping...
    case SDL_WINDOWEVENT: {
      SDL_Event *event = &sdl_event;
      NSWindow *window = [NSWindow fromSDLWindowNumber:sdl_event.window.windowID];
      if (window) [window handleSDLEvent:(void *)&sdl_event];
      break;
    }
  }

  [_current_event initFromSDL:(void *)&sdl_event];

  return _current_event;
}

- (void)sendEvent:(NSEvent *)event {
  // TODO: use other events to fix isActive, isHidden etc. ?
  if (event == Nil) return;
  NSWindow *window = [event window];
  if (window == Nil) return;
  [window sendEvent:event];
}

- (void)updateWindows {
  // TODO:
  // When this method begins, it posts an NSApplicationWillUpdateNotification to the default notification center.
  // When it successfully completes, it posts an NSApplicationDidUpdateNotification.
}

- (void)finishLaunching {
  // TODO: do something here
  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:@"NSApplicationDidFinishLaunchingNotification" object:self];
}

- (BOOL)isActive {
  // TODO: use window here maybe?
  return YES;
}

- (BOOL)isHidden {
  // TODO: check main window maybe?
  return NO;
}

- (void)setPresentationOptions:(NSApplicationPresentationOptions)options {
  // NOTE: i don't think we really care about this
}

- (BOOL)setActivationPolicy:(NSApplicationActivationPolicy)activationPolicy {
  // NOTE: probably dont care about this
  return YES;
}

- (void)activateIgnoringOtherApps:(BOOL)flag {
  // TODO: call SDL_RaiseWindow here?
}

@end
