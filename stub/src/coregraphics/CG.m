#include "CG.h"
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFDictionary.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_mouse.h>

@interface CGDisplayMode : NSObject {
@public
  SDL_DisplayMode mode;
}
@end
@implementation CGDisplayMode
@end
typedef CGDisplayMode *CGDisplayModeRef;
typedef CFTypeRef CVDisplayLinkRef;

void *CGLGetCurrentContext() {
  // NOTE: this is only used with CGLEnable
  return NULL;
}

CGError CGLEnable(void *context, unsigned feature) {
  // NOTE: used to enable multi-threaded engine with "CGLCEMPEngine"
  // We do not particulary care about that...
  return 0;
}

CGSize CGDisplayScreenSize(CGDirectDisplayID displayId) {
  SDL_DisplayMode tmp;
  if (SDL_GetDesktopDisplayMode(displayId, &tmp)) return (CGSize){0.0, 0.0};
  float hdpi = 72, vdpi = 72;
  if (SDL_GetDisplayDPI(displayId, NULL, &hdpi, &vdpi)) {
    // estimate to 72 dpi just like apple says we should
    hdpi = 72;
    vdpi = 72;
  }
  // Convert resolution to dots per mm
  CGSize size;
  size.width = tmp.w;
  size.height = tmp.h;
  size.width /= hdpi * 25.4;
  size.height /= vdpi * 25.4;
  return size;
}

CGRect CGDisplayBounds(CGDirectDisplayID displayIndex) {
  SDL_Rect rect;
  if (SDL_GetDisplayBounds(displayIndex, &rect)) return (CGRect){0.0, 0.0, 0.0, 0.0};
  return (CGRect){rect.x, rect.y, rect.w, rect.h};
}

CFArrayRef CGDisplayCopyAllDisplayModes(CGDirectDisplayID displayIndex, CFDictionaryRef options) {

  int modes = SDL_GetNumDisplayModes(displayIndex);
  if (modes < 1) return NULL;
  NSMutableArray *array = [NSMutableArray arrayWithCapacity:modes];
  for (int i = 0; i < modes; ++i) {
    SDL_DisplayMode mode;
    if (SDL_GetDisplayMode(displayIndex, i, &mode)) continue;

    CGDisplayMode *result = [CGDisplayMode new];
    result->mode = mode;
    [array addObject:result];
  }
  return (CFArrayRef)array;
}

CGDisplayModeRef CGDisplayCopyDisplayMode(CGDirectDisplayID displayId) {
  SDL_DisplayMode mode;
  if (SDL_GetCurrentDisplayMode(displayId, &mode)) {
    return NULL;
  }
  CGDisplayMode *result = [CGDisplayMode new];
  result->mode = mode;
  return result;
}

CFStringRef CGDisplayModeCopyPixelEncoding(CGDisplayModeRef mode) {
  return (CFStringRef)@IO32BitDirectPixels;
}

size_t CGDisplayModeGetHeight(CGDisplayModeRef mode) {
  if (!mode) return 0;
  return ((CGDisplayMode *)mode)->mode.h;
}

size_t CGDisplayModeGetWidth(CGDisplayModeRef mode) {
  if (!mode) return 0;
  return ((CGDisplayMode *)mode)->mode.w;
}

double CGDisplayModeGetRefreshRate(CGDisplayModeRef mode) {
  if (!mode) return 0.0;
  return ((CGDisplayMode *)mode)->mode.refresh_rate;
}

CVReturn CVDisplayLinkCreateWithCGDisplay(CGDirectDisplayID displayID, CVDisplayLinkRef *displayLinkOut) {
  // NOTE: only used as a fallback for when CGDisplayModeGetRefreshRate returns 0
  return kCVReturnSuccess;
}

CVTime CVDisplayLinkGetNominalOutputVideoRefreshPeriod(CVDisplayLinkRef displayLink) {
  // NOTE: only used as a fallback for when CGDisplayModeGetRefreshRate returns 0
  CVTime time;
  time.timeValue = 60;
  time.timeScale = 1;
  time.flags = 0;
  return time;
}

void CGDisplayModeRelease(CGDisplayModeRef mode) {
  if (mode != NULL) CFRelease(mode);
}

CGDisplayModeRef CGDisplayModeRetain(CGDisplayModeRef mode) {
  if (mode) CFRetain(mode);
  return mode;
}

CGError CGDisplaySetDisplayMode(CGDirectDisplayID displayId, CGDisplayModeRef mode, CFDictionaryRef options) {
  return kCGErrorSuccess;
}

CGError CGGetActiveDisplayList(uint32_t maxDisplays, CGDirectDisplayID *activeDisplays, uint32_t *displayCount) {
  int displays = SDL_GetNumVideoDisplays();
  *displayCount = 0;
  if (displays < 1) return kCGErrorFailure;
  for (int i = 0; i < displays && i < maxDisplays; ++i) {
    activeDisplays[i] = i;
    *displayCount += 1;
  }
  return kCGErrorSuccess;
}

CGDirectDisplayID CGMainDisplayID(void) {
  // SDL puts main display id at 0
  return 0;
}

io_service_t CGDisplayIOServicePort(CGDirectDisplayID displayID) {
  return displayID;
}

CFDictionaryRef IODisplayCreateInfoDictionary(uintptr_t framebuffer, IOOptionBits options) {
  // TODO: this crashes somehow, why?
  return (CFDictionaryRef)[@{
    //@"DisplayProductName": Nil,
  } retain];
}

CGError CGAssociateMouseAndMouseCursorPosition(boolean_t connected) { 
  // FIXME
  return kCGErrorSuccess;
}

CGError CGWarpMouseCursorPosition(CGPoint newCursorPosition) {
  // FIXME
  // SDL_WarpMouseGlobal(newCursorPosition.x, newCursorPosition.y);
  return kCGErrorSuccess;
}

CGError CGSetLocalEventsSuppressionInterval(CFTimeInterval a) {
  return kCGErrorSuccess;
}

unsigned CGShieldingWindowLevel() {
  // NOTE: used to detect and set fullscreen?
  return 1000;
}

CGError CGDisplayCapture(CGDirectDisplayID display) {
  // TODO: this prevents other programs to resize, maybe linux has something as well
  return kCGErrorSuccess;
}

CGError CGDisplayRelease(CGDirectDisplayID display) {
  // TODO: See CGDisplayCapture
  return kCGErrorSuccess;
}

CGError CGReleaseAllDisplays(void) {
  // TODO: See CGDisplayCapture
  return kCGErrorSuccess;
}

// This is linked externally, no better place to put it than here...
CGRect const $darwin$NSZeroRect = {{0.0, 0.0}, {0.0, 0.0}};
