#include "NSOpenGLContext.h"
#include "NSView.h"
#include "NSWindow.h"
#include <common.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_error.h>

static _Thread_local NSOpenGLContext *_currentContext = NULL;

@implementation NSOpenGLContext {
  NSView* _view;
  SDL_GLContext _context;
  SDL_Window* _sdl_window;
  NSOpenGLContext* _share;
}
- (instancetype)initWithFormat:(NSOpenGLPixelFormat *)format shareContext:(NSOpenGLContext *)share {
  _share = [share retain];
  fprintf(stderr, "[NSOpenGLContext] init %p shareContext=%p\n", self, share);
  return self;
}

- (void *)_sdlGLContext {
  return _context;
}
- (void *)_sdlWindow {
  return _sdl_window;
}

+ (void)clearCurrentContext {
  _currentContext = NULL;
  SDL_GL_MakeCurrent(NULL, NULL);
}
+ (NSOpenGLContext *)currentContext {
  return _currentContext;
}

// Creates the backing SDL context on first use. A context built with shareContext: has
// no view of its own -- it borrows the parent's window and is created with GL object
// sharing so worker threads can upload into the same GL namespace. Without this the
// worker's uploads go nowhere and anything waiting on them (streaming fences) never
// completes.
- (BOOL)_ensureContext {
  if (_context) return YES;

  SDL_Window* window = NULL;
  if (_view) {
    window = [[_view window] getSDLWindow];
  } else if (_share) {
    window = (SDL_Window*)[_share _sdlWindow];
  }
  if (!window) return NO;

  SDL_GLContext share_ctx = _share ? (SDL_GLContext)[_share _sdlGLContext] : NULL;
  if (share_ctx) {
    // SDL derives sharing from whatever context is current at creation time.
    SDL_GLContext const prev = SDL_GL_GetCurrentContext();
    SDL_Window* const prev_window = SDL_GL_GetCurrentWindow();
    SDL_GL_MakeCurrent(window, share_ctx);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    _context = SDL_GL_CreateContext(window);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    SDL_GL_MakeCurrent(prev_window, prev);
    fprintf(stderr, "[NSOpenGLContext] %p shared context = %p (parent %p): %s\n", self, _context, share_ctx,
            _context ? "ok" : SDL_GetError());
  } else {
    _context = SDL_GL_CreateContext(window);
    fprintf(stderr, "[NSOpenGLContext] %p context = %p: %s\n", self, _context, _context ? "ok" : SDL_GetError());
  }
  if (!_context) return NO;
  _sdl_window = window;
  return YES;
}

- (void)makeCurrentContext {
  if (![self _ensureContext]) {
    fprintf(stderr, "[NSOpenGLContext] %p makeCurrentContext with no backing context!\n", self);
    return;
  }
  _currentContext = self;
  SDL_GL_MakeCurrent(_sdl_window, _context);
}

- (NSView *)view {
  return _view;
}
- (void)setView:(NSView *)view {
  printf("set gl view: %p\n", view);
  _view = view;
}

- (void)update {
  [self _ensureContext];
}

- (void)flushBuffer {
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
