#include "NSOpenGLContext.h"
#include "NSView.h"
#include "NSWindow.h"
#include <common.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_error.h>

@implementation NSOpenGLContext {
  NSView* _view;
  SDL_GLContext _context;
  SDL_Window* _sdl_window;
}
- (instancetype)initWithFormat:(NSOpenGLPixelFormat *)format shareContext:(NSOpenGLContext *)share {
  // panic("Unimplemented");
  return self;
}

+ (void)clearCurrentContext {
  panic("Unimplemented");
}
+ (NSOpenGLContext *)currentContext {
  panic("Unimplemented");
}
- (void)makeCurrentContext {
  // panic("Unimplemented");
  SDL_Window* window = [[_view window] getSDLWindow];
  SDL_GL_MakeCurrent(window, _context);
}

- (NSView *)view {
  return _view;
}
- (void)setView:(NSView *)view {
//   panic("Unimplemented");
  printf("set gl view: %p\n", view);
  _view = view;
}

- (void)update {
  // panic("Unimplemented");
  if (!_context && _view) {
    _sdl_window = [[_view window] getSDLWindow];
    _context = SDL_GL_CreateContext(_sdl_window);
    printf("SDL error: %s, window = %p\n", SDL_GetError(), _sdl_window);
    assert_that(_context != NULL);
  } 
}

- (void)flushBuffer {
  // panic("Unimplemented");
  if (_sdl_window) {
    SDL_GL_SwapWindow(_sdl_window);
  }
}

- (void)getValues:(GLint *)vals forParameter:(NSOpenGLContextParameter)parameter {
  // panic("Unimplemented");
}
- (void)setValues:(const GLint *)vals forParameter:(NSOpenGLContextParameter)param {
  // panic("Unimplemented");
}

@end

@implementation NSOpenGLPixelFormat
- (instancetype)initWithAttributes:(const NSOpenGLPixelFormatAttribute *)attribs {
  // panic("Unimplemented");
  // FIXME actually copy attributes here ...
  return self;
}
- (id)initWithData:(NSData *)attribs {
  panic("Unimplemented");
}
- (NSData *)attributes {
  panic("Unimplemented");
}
- (void)setAttributes:(NSData *)attribs {
  panic("Unimplemented");
}
@end
