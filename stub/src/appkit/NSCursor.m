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
@end
