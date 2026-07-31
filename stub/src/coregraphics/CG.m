#include "CG.h"
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFDictionary.h>
#include <SDL2/SDL_rect.h>
#include "corefoundation/minicf.h"
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_stdinc.h>

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
  NSMutableArray *array = [NSMutableArray arrayWithCapacity:modes > 0 ? modes : 1];
  for (int i = 0; i < modes; ++i) {
    SDL_DisplayMode mode;
    if (SDL_GetDisplayMode(displayIndex, i, &mode)) continue;

    CGDisplayMode *result = [CGDisplayMode new];
    result->mode = mode;
    [array addObject:result];
  }
  // Dummy backends (Xvfb) enumerate zero modes; the game feeds the result straight into
  // CFArrayGetCount with no NULL/empty check, so guarantee at least one entry -- the
  // current mode if SDL knows it, otherwise a plain 1024x768@60 fallback.
  if ([array count] == 0) {
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(displayIndex, &mode) && SDL_GetDesktopDisplayMode(displayIndex, &mode)) {
      mode = (SDL_DisplayMode){SDL_PIXELFORMAT_RGB888, 1024, 768, 60, NULL};
    }
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
CGPoint const $darwin$NSZeroPoint = {0.0, 0.0};
CGSize const $darwin$NSZeroSize = {0.0, 0.0};
CGPoint const CGPointZero = {0.0, 0.0};

// ---------------------------------------------------------------------------
// Geometry helpers -- inline on Apple, real exports in the shipped framework.
// ---------------------------------------------------------------------------

double CGRectGetWidth(CGRect rect) {
  return rect.size.width;
}

double CGRectGetHeight(CGRect rect) {
  return rect.size.height;
}

CGRect CGRectIntersection(CGRect a, CGRect b) {
  double const x0 = a.origin.x > b.origin.x ? a.origin.x : b.origin.x;
  double const y0 = a.origin.y > b.origin.y ? a.origin.y : b.origin.y;
  double const ax1 = a.origin.x + a.size.width, bx1 = b.origin.x + b.size.width;
  double const ay1 = a.origin.y + a.size.height, by1 = b.origin.y + b.size.height;
  double const x1 = ax1 < bx1 ? ax1 : bx1;
  double const y1 = ay1 < by1 ? ay1 : by1;
  if (x1 <= x0 || y1 <= y0) return (CGRect){{__builtin_inf(), __builtin_inf()}, {0.0, 0.0}};  // CGRectNull
  return (CGRect){{x0, y0}, {x1 - x0, y1 - y0}};
}

// ---------------------------------------------------------------------------
// Display geometry / configuration
// ---------------------------------------------------------------------------

size_t CGDisplayPixelsWide(CGDirectDisplayID display) {
  return (size_t)CGDisplayBounds(display).size.width;
}

size_t CGDisplayPixelsHigh(CGDirectDisplayID display) {
  return (size_t)CGDisplayBounds(display).size.height;
}

CGError CGGetDisplaysWithPoint(CGPoint point, uint32_t maxDisplays, CGDirectDisplayID *displays, uint32_t *count) {
  // Single logical display; anything inside its bounds belongs to it.
  if (count) *count = 0;
  if (!maxDisplays || !displays) return kCGErrorSuccess;
  CGRect const b = CGDisplayBounds(CGMainDisplayID());
  if (point.x < b.origin.x || point.y < b.origin.y || point.x >= b.origin.x + b.size.width ||
      point.y >= b.origin.y + b.size.height)
    return kCGErrorSuccess;
  displays[0] = CGMainDisplayID();
  if (count) *count = 1;
  return kCGErrorSuccess;
}

CGError CGGetDisplaysWithRect(CGRect rect, uint32_t maxDisplays, CGDirectDisplayID *displays, uint32_t *count) {
  if (count) *count = 0;
  if (!maxDisplays || !displays) return kCGErrorSuccess;
  CGRect const isect = CGRectIntersection(rect, CGDisplayBounds(CGMainDisplayID()));
  if (isect.size.width <= 0.0 || isect.size.height <= 0.0) return kCGErrorSuccess;
  displays[0] = CGMainDisplayID();
  if (count) *count = 1;
  return kCGErrorSuccess;
}

// Deprecated mode API (CFDictionary-based). The modern CGDisplayCopyAllDisplayModes
// path above is what the game prefers; this one only has to be non-fatal.
CFArrayRef CGDisplayAvailableModes(CGDirectDisplayID display) {
  return NULL;
}

CFDictionaryRef CGDisplayCurrentMode(CGDirectDisplayID display) {
  return NULL;
}

CGError CGBeginDisplayConfiguration(void **config) {
  if (config) *config = (void *)1;
  return kCGErrorSuccess;
}

CGError CGConfigureDisplayMode(void *config, CGDirectDisplayID display, CFDictionaryRef mode) {
  return kCGErrorSuccess;
}

CGError CGCompleteDisplayConfiguration(void *config, uint32_t option) {
  return kCGErrorSuccess;
}

CGError CGCaptureAllDisplays(void) {
  return kCGErrorSuccess;
}

boolean_t CGDisplayIsCaptured(CGDirectDisplayID display) {
  return 0;
}

CGError CGDisplayRegisterReconfigurationCallback(void *callback, void *userInfo) {
  // Displays never change here, so a registration that fires nothing is correct.
  return kCGErrorSuccess;
}

CGError CGDisplayRemoveReconfigurationCallback(void *callback, void *userInfo) {
  return kCGErrorSuccess;
}

// Gamma ramps: X11 has no per-app equivalent worth wiring up, and the game only uses
// them for its brightness slider.
CGError CGGetDisplayTransferByTable(CGDirectDisplayID display, uint32_t capacity, float *red, float *green, float *blue,
                                    uint32_t *sampleCount) {
  if (sampleCount) *sampleCount = 0;
  return kCGErrorSuccess;
}

CGError CGSetDisplayTransferByTable(CGDirectDisplayID display, uint32_t tableSize, const float *red, const float *green,
                                    const float *blue) {
  return kCGErrorSuccess;
}

void CGDisplayRestoreColorSyncSettings(void) {
}

// Fade-out on mode switch. Reservations succeed and fades complete instantly.
CGError CGAcquireDisplayFadeReservation(float seconds, uint32_t *token) {
  if (token) *token = 1;
  return kCGErrorSuccess;
}

CGError CGReleaseDisplayFadeReservation(uint32_t token) {
  return kCGErrorSuccess;
}

CGError CGDisplayFade(uint32_t token, float duration, float startBlend, float endBlend, float red, float green,
                      float blue, boolean_t synchronous) {
  return kCGErrorSuccess;
}

boolean_t CGCursorIsVisible(void) {
  return SDL_ShowCursor(-1 /* SDL_QUERY */) == 1 /* SDL_ENABLE */;
}

void CGSSetDebugOptions(int options) {
  // Private WindowServer debug knob.
}

// ---------------------------------------------------------------------------
// CGEvent -- the game taps events to detect key repeats it did not ask for. SDL
// already owns input, so there is no tap and no source.
// ---------------------------------------------------------------------------

void *CGEventCreateMouseEvent(void *source, uint32_t mouseType, CGPoint mouseCursorPosition, uint32_t mouseButton) {
  return NULL;
}

uint64_t CGEventGetFlags(void *event) {
  return 0;
}

int64_t CGEventGetIntegerValueField(void *event, uint32_t field) {
  return 0;
}

void *CGEventSourceCreate(int32_t stateID) {
  return NULL;
}

uint64_t CGEventSourceFlagsState(int32_t stateID) {
  return 0;
}

void *CGEventTapCreate(uint32_t tap, uint32_t place, uint32_t options, uint64_t eventsOfInterest, void *callback,
                       void *userInfo) {
  return NULL;
}

void CGEventTapEnable(void *tap, boolean_t enable) {
}

boolean_t CGEventTapIsEnabled(void *tap) {
  return 0;
}

// ---------------------------------------------------------------------------
// Bitmap contexts / images -- used for the software cursor and the crash screenshot.
// Nothing consumes the results, so every constructor politely fails and the
// destructors accept NULL.
// ---------------------------------------------------------------------------

void *CGColorSpaceCreateDeviceRGB(void) {
  return NULL;
}

void CGColorSpaceRelease(void *space) {
}

void *CGBitmapContextCreate(void *data, size_t width, size_t height, size_t bitsPerComponent, size_t bytesPerRow,
                            void *space, uint32_t bitmapInfo) {
  return NULL;
}

void CGContextRelease(void *c) {
}

void CGContextFillRect(void *c, CGRect rect) {
}

void CGContextSetFillColor(void *c, const double *components) {
}

void CGContextSetFillColorSpace(void *c, void *space) {
}

void CGContextSetBlendMode(void *c, int32_t mode) {
}

void CGContextSetAllowsAntialiasing(void *c, boolean_t allowsAntialiasing) {
}

void CGContextSetShouldAntialias(void *c, boolean_t shouldAntialias) {
}

void CGContextSetInterpolationQuality(void *c, int32_t quality) {
}

void *CGDataProviderCreateWithCFData(CFDataRef data) {
  return NULL;
}

void CGDataProviderRelease(void *provider) {
}

void *CGImageCreate(size_t width, size_t height, size_t bitsPerComponent, size_t bitsPerPixel, size_t bytesPerRow,
                    void *space, uint32_t bitmapInfo, void *provider, const double *decode, boolean_t shouldInterpolate,
                    int32_t intent) {
  return NULL;
}

void CGImageRelease(void *image) {
}
