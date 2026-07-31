#include "NSWindow.h"
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_video.h>
#include "Foundation/NSArray.h"
#include "Foundation/NSGeometry.h"
#include "NSApplication.h"
#include "NSResponder.h"
#include "NSScreen.h"
#include "NSView.h"
#include <common.h>
#include "enums.h"
#include "input_debug.h"

static NSUInteger NSToSDL(unsigned flags) {
  NSUInteger style = 0;
  if (flags & SDL_WINDOW_FULLSCREEN) {
    style = NSWindowStyleMaskBorderless;
  } else {
    style = NSWindowStyleMaskMiniaturizable;
    if (flags & SDL_WINDOW_BORDERLESS) {
      style |= NSWindowStyleMaskBorderless;
    } else {
      style |= (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable);
    }
    if (flags & SDL_WINDOW_RESIZABLE) {
      style |= NSWindowStyleMaskResizable;
    }
  }
  return style;
}

static unsigned NSWindowStyleMask_to_sdl(NSWindowStyleMask mask) {
  unsigned flags = SDL_WINDOW_OPENGL;
  if (!(flags & (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable))) flags |= SDL_WINDOW_BORDERLESS;
  if (flags & NSWindowStyleMaskResizable) flags |= SDL_WINDOW_RESIZABLE;
  return flags;
}

static NSRect rect_to_sdl(NSRect rect) {
  SDL_Rect total = {};
  SDL_GetDisplayBounds(0, &total);
  return (NSRect){
      rect.origin.x,
      total.h - rect.size.height - rect.origin.y,
      rect.size.width,
      rect.size.height,
  };
}

static NSWindow *_key_window = Nil;

@implementation NSWindow {
  SDL_Window *_sdl_window;
  id _contentView;
  id _orgView;
  id _delegate;
  id _firstResponder;
  NSWindowStyleMask _style;
  NSRect _frame;
  NSWindowLevel _level;
}

- (void *)getSDLWindow {
  return _sdl_window;
}

+ (NSWindow *)keyWindow {
  return _key_window;
}

+ (NSWindow *)fromSDLWindowNumber:(NSUInteger)num {
  // FIXME: this is racy
  if (num == -1) return nil;

  SDL_Window *sdl_window = SDL_GetWindowFromID(num);
  if (sdl_window == NULL) return nil;

  void *data = SDL_GetWindowData(sdl_window, "NSWindow");
  if (data == NULL) return nil;

  return (NSWindow *)data;
}

- (void)handleSDLEvent:(void *)sdl_event {
  if (sdl_event == NULL) return;
  SDL_Event *event = (SDL_Event *)sdl_event;
  switch (event->type) {
    case SDL_WINDOWEVENT:
      switch (event->window.event) {
        case SDL_WINDOWEVENT_SHOWN:
          break;
        case SDL_WINDOWEVENT_HIDDEN:
          break;
        case SDL_WINDOWEVENT_EXPOSED:
          break;
        case SDL_WINDOWEVENT_MOVED: {
          NSRect tmp = [self contentRectForFrameRect:_frame];
          tmp.origin = (NSPoint){event->window.data1, event->window.data2};
          tmp = rect_to_sdl(tmp);
          _frame = [self frameRectForContentRect:tmp];
        } break;
        case SDL_WINDOWEVENT_RESIZED: {
          NSRect tmp = [self contentRectForFrameRect:_frame];
          tmp.size = (NSSize){event->window.data1, event->window.data2};
          _frame = [self frameRectForContentRect:tmp];
        } break;
        case SDL_WINDOWEVENT_SIZE_CHANGED: {
        } break;
        case SDL_WINDOWEVENT_MINIMIZED:
          break;
        case SDL_WINDOWEVENT_MAXIMIZED:
          break;
        case SDL_WINDOWEVENT_RESTORED:
          break;
        case SDL_WINDOWEVENT_ENTER:
          break;
        case SDL_WINDOWEVENT_LEAVE:
          break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
          break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
          break;
        case SDL_WINDOWEVENT_CLOSE:
          break;
        case SDL_WINDOWEVENT_TAKE_FOCUS:
          break;
        case SDL_WINDOWEVENT_HIT_TEST:
          break;
        default:
          break;
      }
      break;
  }
}

+ (instancetype)windowWithContentViewController:(NSViewController *)contentViewController {
  panic("Unimplemented");
}
- (instancetype)initWithContentRect:(NSRect)contentRect
                          styleMask:(NSWindowStyleMask)style
                            backing:(NSBackingStoreType)backingStoreType
                              defer:(BOOL)flag {
  printf("initWithContentRect(%f, %f)\n", contentRect.size.width, contentRect.size.height);
  SDL_Rect total = {};
  SDL_GetDisplayBounds(0, &total);

  NSRect rect = rect_to_sdl(contentRect);
  unsigned flags = NSWindowStyleMask_to_sdl(style);
  _style = style;
  _sdl_window = SDL_CreateWindow("Title", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, flags);
  assert_that(_sdl_window != NULL);
  SDL_SetWindowData(_sdl_window, "NSWindow", self);

  _orgView = [[NSView alloc] initWithFrame:contentRect];
  _contentView = _orgView;
  [_orgView setWindow:self];

  _level = NSNormalWindowLevel;
  _frame = contentRect;  // [self frameRectForContentRect: contentRect];

  [self setNextResponder:NSApp];
  return self;
}
- (instancetype)initWithContentRect:(NSRect)contentRect
                          styleMask:(NSWindowStyleMask)style
                            backing:(NSBackingStoreType)backingStoreType
                              defer:(BOOL)flag
                             screen:(NSScreen *)screen {
  panic("Unimplemented");
}

- (id)delegate {
  // panic("Unimplemented");
  return _delegate;
}
- (void)setDelegate:(id)delegate {
  if (delegate == _delegate)  // nothing to do:
    return;

  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  if (_delegate)  // remove old:
    [center removeObserver:_delegate name:nil object:self];

  _delegate = delegate;
#define REGISTER($name)                                         \
  if ([_delegate respondsToSelector:@selector(window##$name:)]) \
  [center addObserver:_delegate selector:@selector(window##$name:) name:@"NSWindow" #$name "Notification" object:self]
  REGISTER(DidBecomeKey);  //
  REGISTER(DidBecomeMain);
  REGISTER(DidChangeBackingProperties);
  REGISTER(DidChangeOcclusionState);
  REGISTER(DidChangeScreen);
  REGISTER(DidChangeScreenProfile);
  REGISTER(DidDeminiaturize);
  REGISTER(DidEndLiveResize);
  REGISTER(DidEndSheet);
  REGISTER(DidEnterFullScreen);
  REGISTER(DidEnterVersionBrowser);
  REGISTER(DidExitFullScreen);
  REGISTER(DidExitVersionBrowser);
  REGISTER(DidExpose);
  REGISTER(DidMiniaturize);
  REGISTER(DidMove);
  REGISTER(DidResignKey);
  REGISTER(DidResignMain);  //
  REGISTER(DidResize);
  REGISTER(DidUpdate);
  REGISTER(WillBeginSheet);
  REGISTER(WillClose);
  REGISTER(WillEnterFullScreen);
  REGISTER(WillEnterVersionBrowser);
  REGISTER(WillExitFullScreen);
  REGISTER(WillExitVersionBrowser);
  REGISTER(WillMiniaturize);
  REGISTER(WillMove);
  REGISTER(WillStartLiveResize);
#undef REGISTER
}

- (NSView *)contentView {
  return _contentView;
}
- (void)setContentView:(NSView *)view {
  // FIXME: also call removeFromSuperview on view
  if (view == _contentView) return;
  if (view == Nil)
    view = _orgView;
  else
    [view setWindow:self];
  _contentView = view;
}

- (NSWindowStyleMask)styleMask {
  // panic("Unimplemented");
  return _style;
}
- (void)setStyleMask:(NSWindowStyleMask)styleMask {
  // panic("Unimplemented");
  if (_style == styleMask) return;
  bool is_boredered = (_style & (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable));
  bool will_boredered = (styleMask & (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable));
  if (is_boredered != will_boredered) {
    SDL_SetWindowBordered(_sdl_window, will_boredered);
  }
  bool is_resizable = (_style & NSWindowStyleMaskResizable);
  bool will_resizable = (styleMask & NSWindowStyleMaskResizable);
  if (is_resizable != will_resizable) {
    SDL_SetWindowResizable(_sdl_window, will_resizable);
  }
  _style = styleMask;
}

- (BOOL)hidesOnDeactivate {
  panic("Unimplemented");
}
- (void)setHidesOnDeactivate:(BOOL)flag {
  // FIXME: hide window here?
  // panic("Unimplemented");
}

- (BOOL)isOpaque {
  float opacity = 1.0f;
  SDL_GetWindowOpacity(_sdl_window, &opacity);
  return opacity >= 0.9;
}
- (void)setOpaque:(BOOL)value {
  SDL_SetWindowOpacity(_sdl_window, value ? 1.0 : 0.0);
}

- (NSDictionary<NSDeviceDescriptionKey, id> *)deviceDescription {
  panic("Unimplemented");
}

- (NSRect)contentRectForFrameRect:(NSRect)rect {
  int top = 0, left = 0, bottom = 0, right = 0;
  SDL_GetWindowBordersSize(_sdl_window, &top, &left, &bottom, &right);
  rect.size.width -= left;
  rect.size.width -= right;
  rect.size.height -= top;
  rect.size.height -= bottom;
  rect.origin.x += left;
  rect.origin.y += bottom;
  return rect;
}
- (NSRect)frameRectForContentRect:(NSRect)rect {
  int top = 0, left = 0, bottom = 0, right = 0;
  SDL_GetWindowBordersSize(_sdl_window, &top, &left, &bottom, &right);
  rect.size.width += left;
  rect.size.width += right;
  rect.size.height += top;
  rect.size.height += bottom;
  rect.origin.x -= left;
  rect.origin.y -= bottom;
  return rect;
}

- (NSRect)frame {
  return _frame;
  int x = 0, y = 0, w = 0, h = 0;
  SDL_GetWindowPosition(_sdl_window, &x, &y);
  SDL_GetWindowSize(_sdl_window, &w, &y);
  return rect_to_sdl((NSRect){{x, y}, {w, h}});
}
- (void)setFrame:(NSRect)frame display:(BOOL)display {
  printf("setFrame(%f, %f)\n", frame.size.width, frame.size.height);
  // panic("Unimplemented");
  if ((int)frame.origin.x != (int)_frame.origin.x || (int)frame.origin.y != (int)_frame.origin.y) {
    NSRect rect = rect_to_sdl(frame);
    SDL_SetWindowPosition(_sdl_window, rect.origin.x, rect.origin.y);
    _frame.origin = frame.origin;
  }
  if ((int)frame.size.width != (int)_frame.size.width || (int)frame.size.height != (int)_frame.size.height) {
    NSRect rect = rect_to_sdl(frame);
    SDL_SetWindowSize(_sdl_window, rect.size.width, rect.size.height);
    _frame.size = frame.size;
  }
}

- (void)orderOut:(id)sender {
  // 16.15 takes the renderer-init failure path through here on its way to the error alert.
  if (_sdl_window) SDL_HideWindow(_sdl_window);
}
- (void)orderBack:(id)sender {
  panic("Unimplemented");
}
- (void)orderFront:(id)sender {
  // panic("Unimplemented");
}

- (NSWindowLevel)level {
  //panic("Unimplemented");
  return _level;
}
- (void)setLevel:(NSWindowLevel)value {
  // panic("Unimplemented");
  // FIXME: do anything here?
  _level = value;
}
- (BOOL)isVisible {
  if (_sdl_window == NULL) return NO;
  return (SDL_GetWindowFlags(_sdl_window) & SDL_WINDOW_SHOWN) ? YES : NO;
}

- (BOOL)canBecomeKeyWindow {
  return NO;
}
- (BOOL)canBecomeMainWindow {
  return NO;
}

- (BOOL)isKeyWindow {
  return _key_window == self;
}

- (void)makeKeyAndOrderFront:(id)sender {
  // FIXME: anything else to do here
  SDL_ShowWindow(_sdl_window);

  if ([self canBecomeKeyWindow] == NO) {
    return;
  }
  _key_window = self;

  if (_firstResponder) {
    [_firstResponder becomeFirstResponder];
    if ((_firstResponder != self) && [_firstResponder respondsToSelector: @selector(becomeKeyWindow)]) {
      [_firstResponder becomeKeyWindow];
    }
  }

  NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:@"NSWindowDidBecomeKeyNotification" object:self];
  [center postNotificationName:@"NSWindowDidBecomeMainNotification" object:self];
}

- (NSEvent *)currentEvent {
  return [NSApp currentEvent];
}
- (NSEvent *)nextEventMatchingMask:(NSEventMask)mask
                         untilDate:(NSDate *)untilDate
                            inMode:(NSString *)mode
                           dequeue:(BOOL)dequeue {
  return [NSApp nextEventMatchingMask:mask untilDate:untilDate inMode:mode dequeue:dequeue];
}
- (void)sendEvent:(NSEvent *)event {
  if (event == nil) return;

  if (input_debug_enabled() && [event type] != 0) {
    INPUT_DEBUG("window(%s) sendEvent: type=%ld first=%s next=%s contentView=%s", class_getName([self class]),
                (long)[event type], _firstResponder ? class_getName([_firstResponder class]) : "(nil)",
                [self nextResponder] ? class_getName([[self nextResponder] class]) : "(nil)",
                _contentView ? class_getName([_contentView class]) : "(nil)");
  }

  switch ([event type]) {
#define HANDLE($e, $m)  \
  case NSEventType##$e: \
    return [self $m:event]
    HANDLE(LeftMouseDown, mouseDown);
    HANDLE(LeftMouseDragged, mouseDragged);
    HANDLE(LeftMouseUp, mouseUp);
    HANDLE(MouseMoved, mouseMoved);
    HANDLE(MouseEntered, mouseEntered);
    HANDLE(MouseExited, mouseExited);
    HANDLE(RightMouseDown, rightMouseDown);
    HANDLE(RightMouseUp, rightMouseUp);
    HANDLE(RightMouseDragged, rightMouseDragged);
    HANDLE(OtherMouseDown, otherMouseDown);
    HANDLE(OtherMouseUp, otherMouseUp);
    HANDLE(OtherMouseDragged, otherMouseDragged);
    HANDLE(KeyDown, keyDown);
    HANDLE(KeyUp, keyUp);
    HANDLE(CursorUpdate, cursorUpdate);
    HANDLE(FlagsChanged, flagsChanged);
    // HANDLE(HelpRequested, helpRequested);
    HANDLE(ScrollWheel, scrollWheel);
    HANDLE(QuickLook, quickLookWithEvent);
    HANDLE(ChangeMode, changeModeWithEvent);
    default: break;
#undef HANDLE
  }
}

- (void)keyDown:(NSEvent *)event {
  // FIXME: hack to dispatch into NSTextView
  NSArray *subviews = [_contentView subviews];
  if ([event type] == NSEventTypeKeyDown) {
    for (unsigned int i = 0; i < [subviews count]; ++i) {
      [[subviews objectAtIndex:i] keyDown: event];
    }  
  }
  [super keyDown: event];
}

- (void)keyUp:(NSEvent *)event {
  [super keyUp: event];
}

- (NSView *)initialFirstResponder {
  panic("Unimplemented");
}
- (void)setInitialFirstResponder:(NSView *)view {
  panic("Unimplemented");
}

- (NSResponder *)firstResponder {
  // panic("Unimplemented");
  return _firstResponder;
}
- (BOOL)makeFirstResponder:(NSResponder *)responder {
  INPUT_DEBUG("makeFirstResponder: %s (current=%s)", responder ? class_getName([responder class]) : "(nil)",
              _firstResponder ? class_getName([_firstResponder class]) : "(nil)");
  // nothing to do
  if (_firstResponder == responder) return YES;
  // ask responder if its willing to be first
  // NOTE: this used to be `responder != nil || ...`, which rejected every
  // non-nil responder, so -firstResponder always stayed nil.
  if (responder != nil && ![responder acceptsFirstResponder]) return NO;
  // ask current first responder if its willing to resign
  if (_firstResponder && (![_firstResponder resignFirstResponder])) return NO;

  _firstResponder = responder;
  // if first responder didn't want became first one, we are now first because last one resigned
  if (responder == nil || ![_firstResponder becomeFirstResponder]) {
    _firstResponder = self;
    [_firstResponder becomeFirstResponder];
  }
  return YES;
}

- (BOOL)acceptsMouseMovedEvents {
  panic("Unimplemented");
}
- (void)setAcceptsMouseMovedEvents:(BOOL)flag {
  // FIXME: make this actually do something
  // panic("Unimplemented");
}

- (BOOL)ignoresMouseEvents {
  panic("Unimplemented");
}
- (void)setIgnoresMouseEvents:(BOOL)value {
  // FIXME: make this actually do something
  // panic("Unimplemented");
}

- (void)display {
  panic("Unimplemented");
}
- (void)update {
  panic("Unimplemented");
}

- (NSRect)convertRectToScreen:(NSRect)rect {
  panic("Unimplemented");
}

- (NSString *)title {
  panic("Unimplemented");
}
- (void)setTitle:(NSString *)title {
  // panic("Unimplemented");
  SDL_SetWindowTitle(_sdl_window, [title UTF8String]);
}

- (NSScreen *)screen {
  // panic("Unimplemented");
  return [NSScreen mainScreen];
}

- (void)close {
  panic("Unimplemented");
}

- (BOOL)isReleasedWhenClosed {
  panic("Unimplemented");
}
- (void)setReleasedWhenClosed:(BOOL)flag {
  panic("Unimplemented");
}

- (void)miniaturize:(id)sender {
  panic("Unimplemented");
}
- (void)deminiaturize:(id)sender {
  panic("Unimplemented");
}

@end
