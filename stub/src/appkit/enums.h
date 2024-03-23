#pragma once
#include <Foundation/NSObject.h>

typedef enum NSBackingStoreType : NSUInteger {
  NSBackingStoreRetained,
  NSBackingStoreNonretained,
  NSBackingStoreBuffered

} NSBackingStoreType;

typedef enum NSInterfaceStyle : NSUInteger {
  NSNoInterfaceStyle = 0,
  NSNextStepInterfaceStyle = 1,
  NSMacintoshInterfaceStyle = 2,
  NSWindows95InterfaceStyle = 3,
} NSInterfaceStyle;

typedef NS_OPTIONS(NSUInteger, NSWindowStyleMask) {
  NSWindowStyleMaskBorderless = 0,
  NSWindowStyleMaskTitled = 1 << 0,
  NSWindowStyleMaskClosable = 1 << 1,
  NSWindowStyleMaskMiniaturizable = 1 << 2,
  NSWindowStyleMaskResizable = 1 << 3,
  NSWindowStyleMaskUtilityWindow = 1 << 4,
  NSWindowStyleMaskDocModalWindow = 1 << 6,
  NSWindowStyleMaskNonactivatingPanel = 1 << 7,
  NSWindowStyleMaskTexturedBackground = 1 << 8,
  NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
  NSWindowStyleMaskHUDWindow = 1 << 13,
  NSWindowStyleMaskFullScreen = 1 << 14,
  NSWindowStyleMaskFullSizeContentView = 1 << 15,
};

typedef enum NSWindowLevel : NSInteger {
  NSNormalWindowLevel = 0,
  NSFloatingWindowLevel = 3,
  NSSubmenuWindowLevel = 3,
  NSTornOffMenuWindowLevel = 3,
  NSMainMenuWindowLevel = 24,
  NSStatusWindowLevel = 25,
  NSModalPanelWindowLevel = 8,
  NSPopUpMenuWindowLevel = 101,
  NSScreenSaverWindowLevel = 1000,
} NSWindowLevel;

typedef enum NSEventType : NSUInteger {
  NSEventTypeLeftMouseDown = 1,
  NSEventTypeLeftMouseUp = 2,
  NSEventTypeRightMouseDown = 3,
  NSEventTypeRightMouseUp = 4,
  NSEventTypeMouseMoved = 5,
  NSEventTypeLeftMouseDragged = 6,
  NSEventTypeRightMouseDragged = 7,
  NSEventTypeMouseEntered = 8,
  NSEventTypeMouseExited = 9,
  NSEventTypeKeyDown = 10,
  NSEventTypeKeyUp = 11,
  NSEventTypeFlagsChanged = 12,
  NSEventTypeAppKitDefined = 13,
  NSEventTypeSystemDefined = 14,
  NSEventTypeApplicationDefined = 15,
  NSEventTypePeriodic = 16,
  NSEventTypeCursorUpdate = 17,
  NSEventTypeScrollWheel = 22,
  NSEventTypeTabletPoint = 23,
  NSEventTypeTabletProximity = 24,
  NSEventTypeOtherMouseDown = 25,
  NSEventTypeOtherMouseUp = 26,
  NSEventTypeOtherMouseDragged = 27,
  NSEventTypeGesture = 29,
  NSEventTypeMagnify = 30,
  NSEventTypeSwipe = 31,
  NSEventTypeRotate = 18,
  NSEventTypeBeginGesture = 19,
  NSEventTypeEndGesture = 20,
  NSEventTypeSmartMagnify = 32,
  NSEventTypeQuickLook = 33,
  NSEventTypePressure = 34,
  NSEventTypeDirectTouch = 37,
  NSEventTypeChangeMode = 38,
} NSEventType;

typedef enum NSEventModifierFlags : NSUInteger {
  NSEventModifierFlagNone = 0,
  NSEventModifierFlagCapsLock = 1 << 16,
  NSEventModifierFlagShift = 1 << 17,
  NSEventModifierFlagControl = 1 << 18,
  NSEventModifierFlagOption = 1 << 19,
  NSEventModifierFlagCommand = 1 << 20,
  NSEventModifierFlagNumericPad = 1 << 21,
  NSEventModifierFlagHelp = 1 << 22,
  NSEventModifierFlagFunction = 1 << 23,
  NSEventModifierFlagDeviceIndependentFlagsMask = 0xffff0000,
} NSEventModifierFlags;

typedef enum NSEventSubtype : short {
  NSEventSubtypeWindowExposed = 0,
  NSEventSubtypeApplicationActivated = 1,
  NSEventSubtypeApplicationDeactivated = 2,
  NSEventSubtypeWindowMoved = 4,
  NSEventSubtypeScreenChanged = 8,
} NSEventSubtype;

typedef enum NSEventMask : unsigned long long {
  NSEventMaskLeftMouseDown = 1ULL << NSEventTypeLeftMouseDown,
  NSEventMaskLeftMouseUp = 1ULL << NSEventTypeLeftMouseUp,
  NSEventMaskRightMouseDown = 1ULL << NSEventTypeRightMouseDown,
  NSEventMaskRightMouseUp = 1ULL << NSEventTypeRightMouseUp,
  NSEventMaskMouseMoved = 1ULL << NSEventTypeMouseMoved,
  NSEventMaskLeftMouseDragged = 1ULL << NSEventTypeLeftMouseDragged,
  NSEventMaskRightMouseDragged = 1ULL << NSEventTypeRightMouseDragged,
  NSEventMaskMouseEntered = 1ULL << NSEventTypeMouseEntered,
  NSEventMaskMouseExited = 1ULL << NSEventTypeMouseExited,
  NSEventMaskKeyDown = 1ULL << NSEventTypeKeyDown,
  NSEventMaskKeyUp = 1ULL << NSEventTypeKeyUp,
  NSEventMaskFlagsChanged = 1ULL << NSEventTypeFlagsChanged,
  NSEventMaskAppKitDefined = 1ULL << NSEventTypeAppKitDefined,
  NSEventMaskSystemDefined = 1ULL << NSEventTypeSystemDefined,
  NSEventMaskApplicationDefined = 1ULL << NSEventTypeApplicationDefined,
  NSEventMaskPeriodic = 1ULL << NSEventTypePeriodic,
  NSEventMaskCursorUpdate = 1ULL << NSEventTypeCursorUpdate,
  NSEventMaskScrollWheel = 1ULL << NSEventTypeScrollWheel,
  NSEventMaskTabletPoint = 1ULL << NSEventTypeTabletPoint,
  NSEventMaskTabletProximity = 1ULL << NSEventTypeTabletProximity,
  NSEventMaskOtherMouseDown = 1ULL << NSEventTypeOtherMouseDown,
  NSEventMaskOtherMouseUp = 1ULL << NSEventTypeOtherMouseUp,
  NSEventMaskOtherMouseDragged = 1ULL << NSEventTypeOtherMouseDragged,
  NSEventMaskGesture = 1ULL << NSEventTypeGesture,
  NSEventMaskMagnify = 1ULL << NSEventTypeMagnify,
  NSEventMaskSwipe = 1ULL << NSEventTypeSwipe,
  NSEventMaskRotate = 1ULL << NSEventTypeRotate,
  NSEventMaskBeginGesture = 1ULL << NSEventTypeBeginGesture,
  NSEventMaskEndGesture = 1ULL << NSEventTypeEndGesture,
  NSEventMaskSmartMagnify = 1ULL << NSEventTypeSmartMagnify,
  NSEventMaskPressure = 1ULL << NSEventTypePressure,
  NSEventMaskDirectTouch = 1ULL << NSEventTypeDirectTouch,
  NSEventMaskChangeMod = 1ULL << NSEventTypeChangeMode,
  NSEventMaskAny = NSUIntegerMax,
} NSEventMask;

typedef enum {
  NSOpenGLCPSwapRectangle = 200,
  NSOpenGLCPSwapRectangleEnable = 201,
  NSOpenGLCPRasterizationEnable = 221,
  NSOpenGLCPSwapInterval = 222,
  NSOpenGLCPSurfaceOrder = 235,
  NSOpenGLCPSurfaceOpacity = 236,
  NSOpenGLCPStateValidation = 301,
} NSOpenGLContextParameter;

typedef enum {
  NSOpenGLPFAAllRenderers = 1,
  NSOpenGLPFADoubleBuffer = 5,
  NSOpenGLPFAStereo = 6,
  NSOpenGLPFAAuxBuffers = 7,
  NSOpenGLPFAColorSize = 8,
  NSOpenGLPFAAlphaSize = 11,
  NSOpenGLPFADepthSize = 12,
  NSOpenGLPFAStencilSize = 13,
  NSOpenGLPFAAccumSize = 14,
  NSOpenGLPFAMinimumPolicy = 51,
  NSOpenGLPFAMaximumPolicy = 52,
  NSOpenGLPFAOffScreen = 53,
  NSOpenGLPFAFullScreen = 54,
  NSOpenGLPFASampleBuffers = 55,
  NSOpenGLPFASamples = 56,
  NSOpenGLPFAAuxDepthStencil = 57,
  NSOpenGLPFAColorFloat = 58,
  NSOpenGLPFAMultisample = 59,
  NSOpenGLPFASupersample = 60,
  NSOpenGLPFASampleAlpha = 61,
  NSOpenGLPFARendererID = 70,
  NSOpenGLPFASingleRenderer = 71,
  NSOpenGLPFANoRecovery = 72,
  NSOpenGLPFAAccelerated = 73,
  NSOpenGLPFAClosestPolicy = 74,
  NSOpenGLPFARobust = 75,
  NSOpenGLPFABackingStore = 76,
  NSOpenGLPFAMPSafe = 78,
  NSOpenGLPFAWindow = 80,
  NSOpenGLPFAMultiScreen = 81,
  NSOpenGLPFACompliant = 83,
  NSOpenGLPFAScreenMask = 84,
  NSOpenGLPFAPixelBuffer = 90,
  NSOpenGLPFAAllowOfflineRenderers = 96,
  NSOpenGLPFAVirtualScreenCount = 128
} NSOpenGLPixelFormatAttribute;

typedef enum NSBitmapFormat {
  NSAlphaFirstBitmapFormat = 1,
  NSAlphaNonpremultipliedBitmapFormat = 2,
  NSFloatingPointSamplesBitmapFormat = 4,
  NS16BitLittleEndianBitmapFormat = (1 << 8),
  NS32BitLittleEndianBitmapFormat = (1 << 9),
  NS16BitBigEndianBitmapFormat = (1 << 10),
  NS32BitBigEndianBitmapFormat = (1 << 11),
  NSBitmapFormatAlphaFirst = 1,
  NSBitmapFormatAlphaNonpremultiplied = (1 << 1),
  NSBitmapFormatFloatingPointSamples = (1 << 2),
  NSBitmapFormatSixteenBitLittleEndian = (1 << 8),
  NSBitmapFormatThirtyTwoBitLittleEndian = (1 << 9),
  NSBitmapFormatSixteenBitBigEndian = (1 << 10),
  NSBitmapFormatThirtyTwoBitBigEndian = (1 << 11)
} NSBitmapFormat;

typedef enum NSApplicationPresentationOptions : NSUInteger {
  NSApplicationPresentationDefault = 0,
  NSApplicationPresentationAutoHideDock = (1 << 0),
  NSApplicationPresentationHideDock = (1 << 1),
  NSApplicationPresentationAutoHideMenuBar = (1 << 2),
  NSApplicationPresentationHideMenuBar = (1 << 3),
  NSApplicationPresentationDisableAppleMenu = (1 << 4),
  NSApplicationPresentationDisableProcessSwitching = (1 << 5),
  NSApplicationPresentationDisableForceQuit = (1 << 6),
  NSApplicationPresentationDisableSessionTermination = (1 << 7),
  NSApplicationPresentationDisableHideApplication = (1 << 8),
  NSApplicationPresentationDisableMenuBarTransparency = (1 << 9),
  NSApplicationPresentationFullScreen = (1 << 10),
  NSApplicationPresentationAutoHideToolbar = (1 << 11),
  NSApplicationPresentationDisableCursorLocationAssistance = (1 << 12),
} NSApplicationPresentationOptions;

typedef enum NSApplicationActivationPolicy : NSInteger {
  NSApplicationActivationPolicyRegular,
  NSApplicationActivationPolicyAccessory,
  NSApplicationActivationPolicyProhibited,
} NSApplicationActivationPolicy;

typedef enum NSApplicationTerminateReply : NSUInteger {
  NSTerminateNow = 1,
  NSTerminateCancel = 0,
  NSTerminateLater = 2,
} NSApplicationTerminateReply;

typedef enum NSApplicationPrintReply : NSUInteger {
  NSPrintingCancelled = 0,
  NSPrintingSuccess = 1,
  NSPrintingReplyLater = 2,
  NSPrintingFailure = 3,
} NSApplicationPrintReply;
