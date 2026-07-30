#include "NSEvent.h"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_video.h>
#include <time.h>
#include "NSWindow.h"
#include <common.h>
#include "enums.h"
#include "input_debug.h"

static unsigned short rev_scan_code(unsigned int code) {
  switch (code) {
    case SDL_SCANCODE_A:
      return 0;
    case SDL_SCANCODE_S:
      return 1;
    case SDL_SCANCODE_D:
      return 2;
    case SDL_SCANCODE_F:
      return 3;
    case SDL_SCANCODE_H:
      return 4;
    case SDL_SCANCODE_G:
      return 5;
    case SDL_SCANCODE_Z:
      return 6;
    case SDL_SCANCODE_X:
      return 7;
    case SDL_SCANCODE_C:
      return 8;
    case SDL_SCANCODE_V:
      return 9;
    case SDL_SCANCODE_NONUSBACKSLASH:
      return 10;
    case SDL_SCANCODE_B:
      return 11;
    case SDL_SCANCODE_Q:
      return 12;
    case SDL_SCANCODE_W:
      return 13;
    case SDL_SCANCODE_E:
      return 14;
    case SDL_SCANCODE_R:
      return 15;
    case SDL_SCANCODE_Y:
      return 16;
    case SDL_SCANCODE_T:
      return 17;
    case SDL_SCANCODE_1:
      return 18;
    case SDL_SCANCODE_2:
      return 19;
    case SDL_SCANCODE_3:
      return 20;
    case SDL_SCANCODE_4:
      return 21;
    case SDL_SCANCODE_6:
      return 22;
    case SDL_SCANCODE_5:
      return 23;
    case SDL_SCANCODE_EQUALS:
      return 24;
    case SDL_SCANCODE_9:
      return 25;
    case SDL_SCANCODE_7:
      return 26;
    case SDL_SCANCODE_MINUS:
      return 27;
    case SDL_SCANCODE_8:
      return 28;
    case SDL_SCANCODE_0:
      return 29;
    case SDL_SCANCODE_RIGHTBRACKET:
      return 30;
    case SDL_SCANCODE_O:
      return 31;
    case SDL_SCANCODE_U:
      return 32;
    case SDL_SCANCODE_LEFTBRACKET:
      return 33;
    case SDL_SCANCODE_I:
      return 34;
    case SDL_SCANCODE_P:
      return 35;
    case SDL_SCANCODE_RETURN:
      return 36;
    case SDL_SCANCODE_L:
      return 37;
    case SDL_SCANCODE_J:
      return 38;
    case SDL_SCANCODE_APOSTROPHE:
      return 39;
    case SDL_SCANCODE_K:
      return 40;
    case SDL_SCANCODE_SEMICOLON:
      return 41;
    case SDL_SCANCODE_BACKSLASH:
      return 42;
    case SDL_SCANCODE_COMMA:
      return 43;
    case SDL_SCANCODE_SLASH:
      return 44;
    case SDL_SCANCODE_N:
      return 45;
    case SDL_SCANCODE_M:
      return 46;
    case SDL_SCANCODE_PERIOD:
      return 47;
    case SDL_SCANCODE_TAB:
      return 48;
    case SDL_SCANCODE_SPACE:
      return 49;
    case SDL_SCANCODE_GRAVE:
      return 50;
    case SDL_SCANCODE_BACKSPACE:
      return 51;
    case SDL_SCANCODE_KP_ENTER:
      return 52;
    case SDL_SCANCODE_ESCAPE:
      return 53;
    case SDL_SCANCODE_RGUI:
      // FIXME: return 63 maybe?
      return 54;
    case SDL_SCANCODE_LGUI:
      return 55;
    case SDL_SCANCODE_LSHIFT:
      return 56;
    case SDL_SCANCODE_CAPSLOCK:
      return 57;
    case SDL_SCANCODE_LALT:
      return 58;
    case SDL_SCANCODE_LCTRL:
      return 59;
    case SDL_SCANCODE_RSHIFT:
      return 60;
    case SDL_SCANCODE_RALT:
      return 61;
    case SDL_SCANCODE_RCTRL:
      return 62;
    case SDL_SCANCODE_F17:
      return 64;
    case SDL_SCANCODE_KP_PERIOD:
      return 65;
    case SDL_SCANCODE_UNKNOWN:
      return 66;
    case SDL_SCANCODE_KP_MULTIPLY:
      return 67;
    case SDL_SCANCODE_KP_PLUS:
      return 69;
    case SDL_SCANCODE_NUMLOCKCLEAR:
      return 71;
    case SDL_SCANCODE_VOLUMEUP:
      return 72;
    case SDL_SCANCODE_VOLUMEDOWN:
      return 73;
    case SDL_SCANCODE_MUTE:
      return 74;
    case SDL_SCANCODE_KP_DIVIDE:
      return 75;
    case SDL_SCANCODE_KP_MINUS:
      return 78;
    case SDL_SCANCODE_F18:
      return 79;
    case SDL_SCANCODE_F19:
      return 80;
    case SDL_SCANCODE_KP_EQUALS:
      return 81;
    case SDL_SCANCODE_KP_0:
      return 82;
    case SDL_SCANCODE_KP_1:
      return 83;
    case SDL_SCANCODE_KP_2:
      return 84;
    case SDL_SCANCODE_KP_3:
      return 85;
    case SDL_SCANCODE_KP_4:
      return 86;
    case SDL_SCANCODE_KP_5:
      return 87;
    case SDL_SCANCODE_KP_6:
      return 88;
    case SDL_SCANCODE_KP_7:
      return 89;
    case SDL_SCANCODE_KP_8:
      return 91;
    case SDL_SCANCODE_KP_9:
      return 92;
    case SDL_SCANCODE_INTERNATIONAL3:
      return 93;
    case SDL_SCANCODE_INTERNATIONAL1:
      return 94;
    case SDL_SCANCODE_KP_COMMA:
      return 95;
    case SDL_SCANCODE_F5:
      return 96;
    case SDL_SCANCODE_F6:
      return 97;
    case SDL_SCANCODE_F7:
      return 98;
    case SDL_SCANCODE_F3:
      return 99;
    case SDL_SCANCODE_F8:
      return 100;
    case SDL_SCANCODE_F9:
      return 101;
    case SDL_SCANCODE_LANG2:
      return 102;
    case SDL_SCANCODE_F11:
      return 103;
    case SDL_SCANCODE_LANG1:
      return 104;
    case SDL_SCANCODE_PRINTSCREEN:
      return 105;
    case SDL_SCANCODE_F16:
      return 106;
    case SDL_SCANCODE_SCROLLLOCK:
      return 107;
    case SDL_SCANCODE_F10:
      return 109;
    case SDL_SCANCODE_APPLICATION:
      return 110;
    case SDL_SCANCODE_F12:
      return 111;
    case SDL_SCANCODE_PAUSE:
      return 113;
    case SDL_SCANCODE_INSERT:
      return 114;
    case SDL_SCANCODE_HOME:
      return 115;
    case SDL_SCANCODE_PAGEUP:
      return 116;
    case SDL_SCANCODE_DELETE:
      return 117;
    case SDL_SCANCODE_F4:
      return 118;
    case SDL_SCANCODE_END:
      return 119;
    case SDL_SCANCODE_F2:
      return 120;
    case SDL_SCANCODE_PAGEDOWN:
      return 121;
    case SDL_SCANCODE_F1:
      return 122;
    case SDL_SCANCODE_LEFT:
      return 123;
    case SDL_SCANCODE_RIGHT:
      return 124;
    case SDL_SCANCODE_DOWN:
      return 125;
    case SDL_SCANCODE_UP:
      return 126;
    case SDL_SCANCODE_POWER:
      return 127;
    // 76, 77, 70, 108, 112, 68, 90
    default:
      return 128;
  }
}

static bool is_mod(unsigned int code) {
  switch(code) {
    case SDL_SCANCODE_RGUI:
    case SDL_SCANCODE_LGUI:
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_CAPSLOCK:
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RSHIFT:
    case SDL_SCANCODE_RALT:
    case SDL_SCANCODE_RCTRL:
      return 1;
    default:
      return 0;
  }
}

static NSString *keycode_to_str(SDL_Keycode keycode) {
  uint8_t buf[8];
  uint32_t c = keycode;
  if (c <= 0x7F) {
    buf[0] = (uint8_t)c;
    buf[1] = '\0';
  } else if (c <= 0x7FF) {
    buf[0] = 0xC0 | (uint8_t)((c >> 6) & 0x1F);
    buf[1] = 0x80 | (uint8_t)(c & 0x3F);
    buf[2] = '\0';
  } else if (c <= 0xFFFF) {
    buf[0] = 0xE0 | (uint8_t)((c >> 12) & 0x0F);
    buf[1] = 0x80 | (uint8_t)((c >> 6) & 0x3F);
    buf[2] = 0x80 | (uint8_t)(c & 0x3F);
    buf[3] = '\0';
  } else {  // TODO: do error checking here and return NULL
    buf[0] = 0xF0 | (uint8_t)((c >> 18) & 0x07);
    buf[1] = 0x80 | (uint8_t)((c >> 12) & 0x3F);
    buf[2] = 0x80 | (uint8_t)((c >> 6) & 0x3F);
    buf[3] = 0x80 | (uint8_t)(c & 0x3F);
    buf[4] = '\0';
  }
  return [[NSString alloc] initWithUTF8String:(char const *)buf];
}

static NSEventModifierFlags keysym_to_mod(SDL_Keymod mod) {
  NSEventModifierFlags result = 0;
  // TODO: store state somewhere?
  if (mod & KMOD_SHIFT) result |= NSEventModifierFlagShift;
  if (mod & KMOD_CTRL) result |= NSEventModifierFlagControl;
  if (mod & KMOD_CAPS) result |= NSEventModifierFlagCapsLock;
  // TODO: check this
  if (mod & KMOD_ALT) result |= NSEventModifierFlagOption;
  if (mod & KMOD_GUI) result |= NSEventModifierFlagCommand;
  // FIXME: more stuff here?
  if (mod & KMOD_NUM) result |= NSEventModifierFlagNumericPad;
  return result;
}

@implementation NSEvent {
  SDL_Event _sdl_event;
}

- (id)copyWithZone:(NSZone *)zone {
  NSEvent *e = (NSEvent *)NSCopyObject(self, 0, zone);
  e->_sdl_event = _sdl_event;
  return e;
}

- (instancetype)initFromSDL:(void *)sdl_event {
  if (sdl_event == NULL)
    _sdl_event = (SDL_Event){0};
  else
    _sdl_event = *(SDL_Event *)sdl_event;
  return self;
}

- (NSEventType)type {
  switch (_sdl_event.type) {
    case SDL_MOUSEBUTTONDOWN:
      switch (_sdl_event.button.button) {
        case SDL_BUTTON_LEFT:
          return NSEventTypeLeftMouseDown;
        case SDL_BUTTON_RIGHT:
          return NSEventTypeRightMouseDown;
        default:
          return NSEventTypeOtherMouseDown;
      }
    case SDL_MOUSEBUTTONUP:
      switch (_sdl_event.button.button) {
        case SDL_BUTTON_LEFT:
          return NSEventTypeLeftMouseUp;
        case SDL_BUTTON_RIGHT:
          return NSEventTypeRightMouseUp;
        default:
          return NSEventTypeOtherMouseUp;
      }
    case SDL_MOUSEMOTION:
      // AppKit reports motion with a button held as a drag, not as a move.
      if (_sdl_event.motion.state & SDL_BUTTON_LMASK) return NSEventTypeLeftMouseDragged;
      if (_sdl_event.motion.state & SDL_BUTTON_RMASK) return NSEventTypeRightMouseDragged;
      if (_sdl_event.motion.state & (SDL_BUTTON_MMASK | SDL_BUTTON_X1MASK | SDL_BUTTON_X2MASK))
        return NSEventTypeOtherMouseDragged;
      return NSEventTypeMouseMoved;
    case SDL_MOUSEWHEEL:
      return NSEventTypeScrollWheel;
    case SDL_WINDOWEVENT:
      return NSEventTypeAppKitDefined;
    case SDL_KEYDOWN:
      // AppKit never delivers keyDown:/keyUp: for pure modifier keys, only flagsChanged:.
      if (is_mod(_sdl_event.key.keysym.scancode)) return NSEventTypeFlagsChanged;
      return NSEventTypeKeyDown;
    case SDL_KEYUP:
      if (is_mod(_sdl_event.key.keysym.scancode)) return NSEventTypeFlagsChanged;
      return NSEventTypeKeyUp;
    default:
      // FIXME: NSEventTypeAppKitDefined as well?
      // FIXME: looks like a placeholder?
      return 0;
  }
}

- (NSEventSubtype)subtype {
  switch (_sdl_event.type) {
    case SDL_WINDOWEVENT:
      switch (_sdl_event.window.event) {
        case SDL_WINDOWEVENT_MOVED:
          return NSEventSubtypeWindowMoved;
        case SDL_WINDOWEVENT_EXPOSED:
          return NSEventSubtypeWindowExposed;
        case SDL_WINDOWEVENT_DISPLAY_CHANGED:
          return NSEventSubtypeScreenChanged;
        default:
          return 0;
      }
    default:
      return 0;
  }
}

- (NSPoint)locationInWindow {
  switch (_sdl_event.type) {
    case SDL_MOUSEMOTION: {
      // TODO: might need to flip this:
      // void* window = [self sdlWindow];
      // assert_that(window != NULL);
      // int h = 0;
      // SDL_GetWindowSize(window, NULL, &h);
      return (NSPoint){_sdl_event.motion.x, _sdl_event.motion.y};
    }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
      // TODO: might need to flip this:
      // void* window = [self sdlWindow];
      // assert_that(window != NULL);
      // int h = 0;
      // SDL_GetWindowSize(window, NULL, &h);
      return (NSPoint){_sdl_event.button.x, _sdl_event.button.y};
    }
    case SDL_MOUSEWHEEL:
      // Scroll events carry a location too; SDL >= 2.26 gives it to us directly.
      return (NSPoint){_sdl_event.wheel.mouseX, _sdl_event.wheel.mouseY};
    default:
      return (NSPoint){0.0, 0.0};
  }
}

- (NSTimeInterval)timestamp {
  // Convert miliseconds to seconds
  return _sdl_event.common.timestamp / 1000.0;
}

- (void*) sdlWindow {
  // FIXME: this is racy
  NSInteger num = [self windowNumber];
  if (num == -1) return nil;

  SDL_Window* sdl_window = SDL_GetWindowFromID(num);
  if (sdl_window == NULL) return nil;
  return SDL_GetWindowData(sdl_window, "NSWindow");
}

- (NSWindow *)window {
  void* window = [self sdlWindow];
  if (window == NULL) return nil;
  return (NSWindow*)window;
}

- (NSInteger)windowNumber {
  switch (_sdl_event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      return _sdl_event.button.windowID;
    case SDL_MOUSEMOTION:
      return _sdl_event.motion.windowID;
    case SDL_MOUSEWHEEL:
      return _sdl_event.wheel.windowID;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      return _sdl_event.key.windowID;
    case SDL_WINDOWEVENT:
      return _sdl_event.window.windowID;
    case SDL_TEXTINPUT:
      return _sdl_event.text.windowID;
    case SDL_TEXTEDITING:
      return _sdl_event.edit.windowID;
    default:
      // FIXME: maybe return 0 here?
      return -1;
  }
}

- (NSEventModifierFlags)modifierFlags {
  switch (_sdl_event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // keysym.mod is already updated for this key press/release, which is what
      // flagsChanged: consumers expect.
      return keysym_to_mod(_sdl_event.key.keysym.mod);
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEWHEEL:
      // AppKit stamps every event with the modifier state, not just key events.
      return keysym_to_mod(SDL_GetModState());
    default:
      return 0;
  }
}

- (NSString *)characters {
  switch (_sdl_event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      if (!_sdl_event.key.keysym.sym || _sdl_event.key.keysym.sym & SDLK_SCANCODE_MASK) return @"";
      return keycode_to_str(_sdl_event.key.keysym.sym);
    default:
      // NOTE: this usually raises an exception, we instead return Nil
      return Nil;
  }
}

- (unsigned short)keyCode {
  switch (_sdl_event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      return rev_scan_code(_sdl_event.key.keysym.scancode);
    default:
      // NOTE: this usually raises an exception, we instead return invalid scancode
      return 128;
  }
}

- (BOOL)isARepeat {
  switch (_sdl_event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      return _sdl_event.key.repeat ? YES : NO;
    default:
      // NOTE: this usually raises an exception, we instead return false
      return NO;
  }
}

- (NSInteger)buttonNumber {
  switch (_sdl_event.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      switch (_sdl_event.button.button) {
        case SDL_BUTTON_LEFT:  // 1
          return 0;
        case SDL_BUTTON_RIGHT:  // 3
          return 1;
        case SDL_BUTTON_MIDDLE:  // 2
          return 2;
        default:
          return _sdl_event.button.button - 1;
      }
    case SDL_MOUSEMOTION:
      // Dragged events report the button being dragged with.
      if (_sdl_event.motion.state & SDL_BUTTON_LMASK) return 0;
      if (_sdl_event.motion.state & SDL_BUTTON_RMASK) return 1;
      if (_sdl_event.motion.state & SDL_BUTTON_MMASK) return 2;
      if (_sdl_event.motion.state & SDL_BUTTON_X1MASK) return 3;
      if (_sdl_event.motion.state & SDL_BUTTON_X2MASK) return 4;
      return 0;
    default:
      return 0;
  }
}

- (CGFloat)deltaX {
  switch (_sdl_event.type) {
    case SDL_MOUSEWHEEL:
      return _sdl_event.wheel.preciseX * 10.0;
    case SDL_MOUSEMOTION:
      return _sdl_event.motion.xrel;
    default:
      return 0.0;
  }
}

- (CGFloat)deltaY {
  switch (_sdl_event.type) {
    case SDL_MOUSEWHEEL:
      return _sdl_event.wheel.preciseY * 10.0;
    case SDL_MOUSEMOTION:
      return -_sdl_event.motion.yrel;
    default:
      return 0.0;
  }
}

+ (NSEventModifierFlags)modifierFlags {
  SDL_Keymod mod = SDL_GetModState();
  return keysym_to_mod(mod);
}

+ (NSPoint)mouseLocation {
  SDL_Rect bounds = {};
  SDL_GetDisplayBounds(0, &bounds);

  int x = 0, y = 0;
  SDL_GetGlobalMouseState(&x, &y);
  if (input_debug_enabled()) {
    static unsigned long calls = 0;
    if ((calls++ % 256) == 0) INPUT_DEBUG("+[NSEvent mouseLocation] #%lu -> %d,%d", calls, x, bounds.h - y);
  }
  return (NSPoint){x, bounds.h - y};
}

@end
