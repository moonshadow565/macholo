#pragma once
// Metal value types. There is no Metal.framework header on Linux, and the game only
// ever sees these through msgSend, so all we need is the ABI shape: sizes, field order
// and whether a thing lands in an integer register, an SSE register or on the stack.
//
// Only the types that actually appear in a method signature we implement by hand are
// listed. Everything else the engine touches goes through the property-bag fallback in
// MACHOLONullObject, which treats every value as a bare 64-bit word.
#import <Foundation/NSObject.h>

typedef struct {
  NSUInteger width;
  NSUInteger height;
  NSUInteger depth;
} MTLSize;

typedef struct {
  NSUInteger x;
  NSUInteger y;
  NSUInteger z;
} MTLOrigin;

typedef struct {
  MTLOrigin origin;
  MTLSize size;
} MTLRegion;

typedef struct {
  double red;
  double green;
  double blue;
  double alpha;
} MTLClearColor;

typedef struct {
  double originX;
  double originY;
  double width;
  double height;
  double znear;
  double zfar;
} MTLViewport;

typedef struct {
  NSUInteger x;
  NSUInteger y;
  NSUInteger width;
  NSUInteger height;
} MTLScissorRect;

typedef NSUInteger MTLPixelFormat;
typedef NSUInteger MTLTextureType;
typedef NSUInteger MTLTextureUsage;
typedef NSUInteger MTLStorageMode;
typedef NSUInteger MTLCPUCacheMode;
typedef NSUInteger MTLResourceOptions;
typedef NSUInteger MTLPurgeableState;
typedef NSUInteger MTLCommandBufferStatus;

// MTLTextureType
enum {
  MTLTextureType1D = 0,
  MTLTextureType1DArray = 1,
  MTLTextureType2D = 2,
  MTLTextureType2DArray = 3,
  MTLTextureType2DMultisample = 4,
  MTLTextureTypeCube = 5,
  MTLTextureTypeCubeArray = 6,
  MTLTextureType3D = 7,
};

// MTLCommandBufferStatus
enum {
  MTLCommandBufferStatusNotEnqueued = 0,
  MTLCommandBufferStatusEnqueued = 1,
  MTLCommandBufferStatusCommitted = 2,
  MTLCommandBufferStatusScheduled = 3,
  MTLCommandBufferStatusCompleted = 4,
  MTLCommandBufferStatusError = 5,
};

// MTLPixelFormat, only the handful we name ourselves.
enum {
  MTLPixelFormatInvalid = 0,
  MTLPixelFormatBGRA8Unorm = 80,
  MTLPixelFormatBGRA8Unorm_sRGB = 81,
  MTLPixelFormatDepth32Float = 252,
};

static inline MTLSize MTLSizeMake(NSUInteger w, NSUInteger h, NSUInteger d) {
  return (MTLSize){w, h, d};
}

// Full mip chain for a "mipmapped:YES" descriptor, matching Metal's floor(log2(max))+1.
static inline NSUInteger macholo_metal_mip_count(NSUInteger w, NSUInteger h, NSUInteger d) {
  NSUInteger m = w > h ? w : h;
  if (d > m) m = d;
  NSUInteger n = 1;
  while (m > 1) {
    m >>= 1;
    n++;
  }
  return n;
}

typedef void (^MACHOLOCommandBufferHandler)(id commandBuffer);
