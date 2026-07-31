#include "NSApplication.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include "GNUstepBase/GSObjCRuntime.h"
#include "NSEvent.h"
#include "NSWindow.h"
#include "SDL_video.h"
#include <common.h>
#include "input_debug.h"

NSApplication *NSApp = Nil;

// DATA: notification names / run-loop modes the game imports directly. The string
// values match what setDelegate registers observers for below.
NSString *NSApplicationDidBecomeActiveNotification = @"NSApplicationDidBecomeActiveNotification";
NSString *NSApplicationDidChangeOcclusionStateNotification = @"NSApplicationDidChangeOcclusionStateNotification";
NSString *NSApplicationDidChangeScreenParametersNotification = @"NSApplicationDidChangeScreenParametersNotification";
NSString *NSApplicationDidFinishLaunchingNotification = @"NSApplicationDidFinishLaunchingNotification";
NSString *NSApplicationDidResignActiveNotification = @"NSApplicationDidResignActiveNotification";
NSString *NSApplicationWillBecomeActiveNotification = @"NSApplicationWillBecomeActiveNotification";
NSString *NSApplicationWillFinishLaunchingNotification = @"NSApplicationWillFinishLaunchingNotification";
NSString *NSApplicationWillHideNotification = @"NSApplicationWillHideNotification";
NSString *NSApplicationWillResignActiveNotification = @"NSApplicationWillResignActiveNotification";
NSString *NSApplicationWillUnhideNotification = @"NSApplicationWillUnhideNotification";
NSString *NSEventTrackingRunLoopMode = @"NSEventTrackingRunLoopMode";
NSString *NSModalPanelRunLoopMode = @"NSModalPanelRunLoopMode";
NSString *NSStringPboardType = @"NSStringPboardType";

// CODE:
BOOL NSApplicationLoad(void) {
  // Carbon-era bootstrap: "connect to the window server". Nothing to connect to.
  [NSApplication sharedApplication];
  return YES;
}

int NSApplicationMain(int argc, const char *argv[]) {
  // Not expected on the current path (the game drives its own loop); if this fires the
  // bring-up needs a real [NSApp run] equivalent.
  panic("NSApplicationMain called -- implement a run loop for it");
}

void NSBeep(void) {
}

@implementation NSApplication {
  id<NSApplicationDelegate> _delegate;
  NSEvent *_current_event;
}

+ (NSApplication *)sharedApplication {
  // panic("Unimplemented!");
  if (NSApp == nil) {
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    // Alloc through +self so a subclass (WC3's MacApplication) gets an instance of
    // itself, exactly like the real +sharedApplication.
    NSApp = [[[self alloc] init] retain];
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

- (NSWindow *)keyWindow {
  // The game references this selector; without an implementation it would be an
  // unrecognized-selector abort.
  return [NSWindow keyWindow];
}

- (NSWindow *)mainWindow {
  return [NSWindow keyWindow];
}

- (NSEvent *)currentEvent {
  // The last event handed out by nextEventMatchingMask:. The game calls this
  // (and -[NSWindow currentEvent], which forwards here) from its input code, so
  // it must not panic.
  return _current_event;
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

  if (input_debug_enabled()) {
    switch (sdl_event.type) {
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        INPUT_DEBUG("sdl key type=%u scancode=%u sym=%u mod=%u winid=%u -> NSEventType=%ld keyCode=%u",
                    sdl_event.type, sdl_event.key.keysym.scancode, sdl_event.key.keysym.sym,
                    sdl_event.key.keysym.mod, sdl_event.key.windowID, (long)[_current_event type],
                    [_current_event keyCode]);
        break;
      case SDL_MOUSEMOTION:
        INPUT_DEBUG("sdl motion x=%d y=%d rel=%d,%d state=%u winid=%u", sdl_event.motion.x, sdl_event.motion.y,
                    sdl_event.motion.xrel, sdl_event.motion.yrel, sdl_event.motion.state, sdl_event.motion.windowID);
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        INPUT_DEBUG("sdl button type=%u btn=%u x=%d y=%d winid=%u -> NSEventType=%ld", sdl_event.type,
                    sdl_event.button.button, sdl_event.button.x, sdl_event.button.y, sdl_event.button.windowID,
                    (long)[_current_event type]);
        break;
      case SDL_MOUSEWHEEL:
        INPUT_DEBUG("sdl wheel x=%f y=%f dir=%u winid=%u", sdl_event.wheel.preciseX, sdl_event.wheel.preciseY,
                    sdl_event.wheel.direction, sdl_event.wheel.windowID);
        break;
      case SDL_TEXTINPUT:
        INPUT_DEBUG("sdl textinput '%s' winid=%u", sdl_event.text.text, sdl_event.text.windowID);
        break;
      case SDL_WINDOWEVENT:
        INPUT_DEBUG("sdl windowevent ev=%u winid=%u data=%d,%d", sdl_event.window.event, sdl_event.window.windowID,
                    sdl_event.window.data1, sdl_event.window.data2);
        break;
      default:
        break;
    }
  }

  return _current_event;
}

- (void)sendEvent:(NSEvent *)event {
  // TODO: use other events to fix isActive, isHidden etc. ?
  if (event == Nil) return;
  NSWindow *window = [event window];
  if (window == Nil) {
    if (input_debug_enabled() && [event type] != 0)
      INPUT_DEBUG("NSApp sendEvent: type=%ld DROPPED (no window, windowNumber=%ld)", (long)[event type],
                  (long)[event windowNumber]);
    return;
  }
  INPUT_DEBUG("NSApp sendEvent: type=%ld -> window %s", (long)[event type], class_getName([window class]));
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
