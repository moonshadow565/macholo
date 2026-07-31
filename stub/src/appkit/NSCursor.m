#include "NSCursor.h"
#include "Foundation/NSGeometry.h"
#include "NSImage.h"
#include "SDL_events.h"
#include <common.h>
#include <SDL2/SDL_mouse.h>

static NSCursor* _currentCursor = NULL;

@implementation NSCursor {
  SDL_Cursor* _sdl_cursor;
  NSImage* _image;
  NSPoint _hotspot;
}

- (id)initWithImage:(NSImage *)image hotSpot:(NSPoint)hotSpot {
  // panic("Unimplemented");
  _image = image;
  _hotspot = hotSpot;
  return self;
}

- (NSImage *)image {
  return _image;
}
- (NSPoint)hotSpot {
  return _hotspot;
}
+ (void)hide {
  // NOTE: these two were swapped; +hide must hide the pointer, not show it.
  SDL_ShowCursor(SDL_DISABLE);
}
+ (void)unhide {
  SDL_ShowCursor(SDL_ENABLE);
}
+ (void)setHiddenUntilMouseMoves:(BOOL)flag {
  // NOTE: this is useless
}

+ (void)pop {
  // TODO: implement this
  panic("Unimplemented");
}
- (void)pop {
  // TODO: implement this
  panic("Unimplemented");
}
- (void)push {
  // TODO: implement this
  panic("Unimplemented");
}
- (void)set {
  _currentCursor = [self retain];
}

+ (NSCursor *)currentCursor {
  return _currentCursor;
}

// One shared instance per system cursor. They must be distinct objects (the game
// compares them) but there is no distinct appearance to give them: SDL is what
// actually draws the pointer, and the game sets its own image for the in-game one.
+ (NSCursor *)_sharedSystemCursor:(NSCursor **)slot {
  if (*slot == Nil) *slot = [[NSCursor alloc] init];
  return *slot;
}

#define SYSTEM_CURSOR($name)                        \
  +(NSCursor *)$name {                              \
    static NSCursor *instance = Nil;                \
    return [NSCursor _sharedSystemCursor:&instance]; \
  }

SYSTEM_CURSOR(arrowCursor)
SYSTEM_CURSOR(IBeamCursor)
SYSTEM_CURSOR(pointingHandCursor)
SYSTEM_CURSOR(closedHandCursor)
SYSTEM_CURSOR(openHandCursor)
SYSTEM_CURSOR(crosshairCursor)
SYSTEM_CURSOR(resizeLeftRightCursor)
SYSTEM_CURSOR(resizeUpDownCursor)
SYSTEM_CURSOR(operationNotAllowedCursor)
#undef SYSTEM_CURSOR
@end
