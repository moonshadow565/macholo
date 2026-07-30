#include "NSScreen.h"
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_mouse.h>
#include <common.h>

NSDeviceDescriptionKey NSDeviceBitsPerSample = @"NSDeviceBitsPerSample";
NSDeviceDescriptionKey NSDeviceColorSpaceName = @"NSDeviceColorSpaceName";
NSDeviceDescriptionKey NSDeviceIsScreen = @"NSDeviceIsScreen";
NSDeviceDescriptionKey NSDeviceIsPrinter = @"NSDeviceIsPrinter";
NSDeviceDescriptionKey NSDeviceResolution = @"NSDeviceResolution";
NSDeviceDescriptionKey NSDeviceSize = @"NSDeviceSize";
NSDeviceDescriptionKey NSScreenNumber = @"NSScreenNumber";

NSString *NSCalibratedRGBColorSpace = @"NSCalibratedRGBColorSpace";
NSString *NSDeviceRGBColorSpace = @"NSDeviceRGBColorSpace";
NSString *NSImageNameCaution = @"NSCaution";
NSString *NSImageNameInfo = @"NSInfo";
NSString *NSMarkedClauseSegmentAttributeName = @"NSMarkedClauseSegmentAttributeName";
NSString *NSTextInputReplacementRangeAttributeName = @"NSTextInputReplacementRangeAttributeName";
NSString *NSViewFrameDidChangeNotification = @"NSViewFrameDidChangeNotification";

NSString *NSImageNameStopProgressFreestandingTemplate = @"NSImageNameStopProgressFreestandingTemplate";

NSString *NSPasteboardTypeString = @"public.utf8-plain-text";

@implementation NSScreen {
  int _number;
}

-init {
  _number = 0;
  return self;
}

+ (NSScreen *)mainScreen {
  static NSScreen *instance = Nil;
  if (instance == Nil) {
    instance = [[NSScreen alloc] init];
  }
  return instance;
}

+ (NSArray<NSScreen *> *)screens {
  return @[ [NSScreen mainScreen] ];
}

- (NSRect)frame {
  SDL_Rect total = {};
  SDL_GetDisplayBounds(_number, &total);
  return (NSRect){
      total.x,
      total.h,
      total.w,
      total.h,
  };
}

- (NSRect)visibleFrame {
  SDL_Rect total = {};
  SDL_GetDisplayBounds(_number, &total);

  SDL_Rect usable = {};
  SDL_GetDisplayUsableBounds(_number, &usable);

  return (NSRect){
      usable.x,
      total.h - usable.h - usable.y,
      usable.w,
      usable.h,
  };
}

- (NSDictionary<NSDeviceDescriptionKey, id> *)deviceDescription {
  float hdpi = 72, vdpi = 72;
  if (SDL_GetDisplayDPI(_number, NULL, &hdpi, &vdpi)) {
    // estimate to 72 dpi just like apple says we should
    hdpi = 72;
    vdpi = 72;
  }

  return @{
    NSDeviceResolution : [NSValue valueWithSize:NSMakeSize(hdpi, vdpi)],
    NSDeviceSize : [NSValue valueWithSize:[self frame].size],
    NSDeviceColorSpaceName : NSDeviceRGBColorSpace,
    NSDeviceBitsPerSample : @8,
    NSDeviceIsScreen : @YES,
    NSDeviceIsPrinter : @NO,
    NSScreenNumber : [NSNumber numberWithInt: _number],
  };
}
@end
