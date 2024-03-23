#pragma once
#include <common.h>
#include <Foundation/NSArray.h>
#include <Foundation/NSString.h>
#include <Foundation/NSGeometry.h>
#include <Foundation/NSDictionary.h>

#define IO1BitIndexedPixels "P"
#define IO2BitIndexedPixels "PP"
#define IO4BitIndexedPixels "PPPP"
#define IO8BitIndexedPixels "PPPPPPPP"
#define IO16BitDirectPixels "-RRRRRGGGGGBBBBB"
#define IO32BitDirectPixels "--------RRRRRRRRGGGGGGGGBBBBBBBB"
#define kIO30BitDirectPixels "--RRRRRRRRRRGGGGGGGGGGBBBBBBBBBB"
#define kIO64BitDirectPixels "-16R16G16B16"
#define kIO16BitFloatPixels "-16FR16FG16FB16"
#define kIO32BitFloatPixels "-32FR32FG32FB32"
#define kCGNullDirectDisplay 0

typedef int boolean_t;
typedef uint32_t IOOptionBits;
typedef intptr_t io_service_t;

typedef uint32_t CGDirectDisplayID;
typedef double CGRefreshRate;
typedef NSRect CGRect;
typedef NSPoint CGPoint;
typedef NSSize CGSize;

typedef struct CVTime {
  int64_t timeValue;
  int32_t timeScale;
  int32_t flags;
} CVTime;

typedef enum CVReturn {
  kCVReturnSuccess = 0,
  kCVReturnInvalidDisplay = -6670,
} CVReturn;

typedef enum CGError {
  kCGErrorSuccess = 0,
  kCGErrorFailure = 1000,
  kCGErrorIllegalArgument = 1001,
  kCGErrorInvalidConnection = 1002,
  kCGErrorInvalidContext = 1003,
  kCGErrorCannotComplete = 1004,
  kCGErrorNotImplemented = 1006,
  kCGErrorRangeCheck = 1007,
  kCGErrorTypeCheck = 1008,
  kCGErrorInvalidOperation = 1010,
  kCGErrorNoneAvailable = 1011,
} CGError;
