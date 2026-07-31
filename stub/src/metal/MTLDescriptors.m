#import "MTLDescriptors.h"
#import <Foundation/NSDictionary.h>
#import <Foundation/NSValue.h>
#include <common.h>

@implementation MACHOLODescriptor
- (id)copyWithZone:(NSZone *)zone {
  (void)zone;
  return [self retain];
}
@end

@implementation MACHOLODescriptorArray {
  Class _elementClass;
  NSMutableDictionary *_slots;
}

- (instancetype)initWithElementClass:(Class)elementClass {
  if ((self = [super init])) {
    _elementClass = elementClass;
    _slots = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (instancetype)init {
  return [self initWithElementClass:[MACHOLODescriptor class]];
}

- (id)objectAtIndexedSubscript:(NSUInteger)index {
  NSNumber *key = [NSNumber numberWithUnsignedLongLong:index];
  id obj = [_slots objectForKey:key];
  if (obj == nil) {
    obj = [[[_elementClass alloc] init] autorelease];
    [_slots setObject:obj forKey:key];
  }
  return obj;
}

- (void)setObject:(id)obj atIndexedSubscript:(NSUInteger)index {
  NSNumber *key = [NSNumber numberWithUnsignedLongLong:index];
  if (obj) [_slots setObject:obj forKey:key];
  else [_slots removeObjectForKey:key];
}

- (void)dealloc {
  [_slots release];
  [super dealloc];
}
@end

// --- render pass -----------------------------------------------------------

@implementation MTLRenderPassAttachmentDescriptor
@synthesize texture = _texture;
@synthesize resolveTexture = _resolveTexture;
- (void)dealloc {
  [_texture release];
  [_resolveTexture release];
  [super dealloc];
}
@end

@implementation MTLRenderPassColorAttachmentDescriptor
@synthesize clearColor = _clearColor;
@end

@implementation MTLRenderPassDepthAttachmentDescriptor
@synthesize clearDepth = _clearDepth;
- (instancetype)init {
  if ((self = [super init])) _clearDepth = 1.0;
  return self;
}
@end

@implementation MTLRenderPassStencilAttachmentDescriptor
@synthesize clearStencil = _clearStencil;
@end

@implementation MTLRenderPassColorAttachmentDescriptorArray
- (instancetype)init {
  return [super initWithElementClass:[MTLRenderPassColorAttachmentDescriptor class]];
}
@end

@implementation MTLRenderPassDescriptor {
  MTLRenderPassColorAttachmentDescriptorArray *_colorAttachments;
}
@synthesize depthAttachment = _depthAttachment;
@synthesize stencilAttachment = _stencilAttachment;

+ (MTLRenderPassDescriptor *)renderPassDescriptor {
  return [[[self alloc] init] autorelease];
}

- (instancetype)init {
  if ((self = [super init])) {
    _colorAttachments = [[MTLRenderPassColorAttachmentDescriptorArray alloc] init];
    _depthAttachment = [[MTLRenderPassDepthAttachmentDescriptor alloc] init];
    _stencilAttachment = [[MTLRenderPassStencilAttachmentDescriptor alloc] init];
  }
  return self;
}

- (MTLRenderPassColorAttachmentDescriptorArray *)colorAttachments {
  return _colorAttachments;
}

- (void)dealloc {
  [_colorAttachments release];
  [_depthAttachment release];
  [_stencilAttachment release];
  [super dealloc];
}
@end

// --- render pipeline -------------------------------------------------------

@implementation MTLRenderPipelineColorAttachmentDescriptor
@end

@implementation MTLRenderPipelineColorAttachmentDescriptorArray
- (instancetype)init {
  return [super initWithElementClass:[MTLRenderPipelineColorAttachmentDescriptor class]];
}
@end

@implementation MTLRenderPipelineDescriptor {
  MTLRenderPipelineColorAttachmentDescriptorArray *_colorAttachments;
}
@synthesize vertexFunction = _vertexFunction;
@synthesize fragmentFunction = _fragmentFunction;
@synthesize vertexDescriptor = _vertexDescriptor;

- (instancetype)init {
  if ((self = [super init]))
    _colorAttachments = [[MTLRenderPipelineColorAttachmentDescriptorArray alloc] init];
  return self;
}

- (MTLRenderPipelineColorAttachmentDescriptorArray *)colorAttachments {
  return _colorAttachments;
}

- (void)reset {
  [_vertexFunction release], _vertexFunction = nil;
  [_fragmentFunction release], _fragmentFunction = nil;
  [_vertexDescriptor release], _vertexDescriptor = nil;
  [_colorAttachments release];
  _colorAttachments = [[MTLRenderPipelineColorAttachmentDescriptorArray alloc] init];
}

- (void)dealloc {
  [_vertexFunction release];
  [_fragmentFunction release];
  [_vertexDescriptor release];
  [_colorAttachments release];
  [super dealloc];
}
@end

// --- textures / samplers ---------------------------------------------------

@implementation MTLTextureDescriptor
@synthesize textureType = _textureType;
@synthesize pixelFormat = _pixelFormat;
@synthesize width = _width;
@synthesize height = _height;
@synthesize depth = _depth;
@synthesize mipmapLevelCount = _mipmapLevelCount;
@synthesize sampleCount = _sampleCount;
@synthesize arrayLength = _arrayLength;
@synthesize resourceOptions = _resourceOptions;
@synthesize cpuCacheMode = _cpuCacheMode;
@synthesize storageMode = _storageMode;
@synthesize usage = _usage;

- (instancetype)init {
  if ((self = [super init])) {
    _textureType = MTLTextureType2D;
    _pixelFormat = MTLPixelFormatBGRA8Unorm;
    _width = 1;
    _height = 1;
    _depth = 1;
    _mipmapLevelCount = 1;
    _sampleCount = 1;
    _arrayLength = 1;
  }
  return self;
}

+ (MTLTextureDescriptor *)texture2DDescriptorWithPixelFormat:(MTLPixelFormat)pixelFormat
                                                       width:(NSUInteger)width
                                                      height:(NSUInteger)height
                                                   mipmapped:(BOOL)mipmapped {
  MTLTextureDescriptor *d = [[[self alloc] init] autorelease];
  d.textureType = MTLTextureType2D;
  d.pixelFormat = pixelFormat;
  d.width = width;
  d.height = height;
  d.mipmapLevelCount = mipmapped ? macholo_metal_mip_count(width, height, 1) : 1;
  return d;
}

+ (MTLTextureDescriptor *)textureCubeDescriptorWithPixelFormat:(MTLPixelFormat)pixelFormat
                                                          size:(NSUInteger)size
                                                     mipmapped:(BOOL)mipmapped {
  MTLTextureDescriptor *d = [[[self alloc] init] autorelease];
  d.textureType = MTLTextureTypeCube;
  d.pixelFormat = pixelFormat;
  d.width = size;
  d.height = size;
  d.mipmapLevelCount = mipmapped ? macholo_metal_mip_count(size, size, 1) : 1;
  return d;
}

+ (MTLTextureDescriptor *)textureBufferDescriptorWithPixelFormat:(MTLPixelFormat)pixelFormat
                                                           width:(NSUInteger)width
                                                 resourceOptions:(MTLResourceOptions)options
                                                           usage:(MTLTextureUsage)usage {
  MTLTextureDescriptor *d = [[[self alloc] init] autorelease];
  d.pixelFormat = pixelFormat;
  d.width = width;
  d.resourceOptions = options;
  d.usage = usage;
  return d;
}
@end

@implementation MTLSamplerDescriptor
@synthesize lodMinClamp = _lodMinClamp;
@synthesize lodMaxClamp = _lodMaxClamp;
- (instancetype)init {
  if ((self = [super init])) {
    _lodMinClamp = 0.0f;
    _lodMaxClamp = 3.402823466e+38f;
  }
  return self;
}
@end

// --- depth/stencil ---------------------------------------------------------

@implementation MTLStencilDescriptor
@end

@implementation MTLDepthStencilDescriptor
@synthesize frontFaceStencil = _frontFaceStencil;
@synthesize backFaceStencil = _backFaceStencil;

- (instancetype)init {
  if ((self = [super init])) {
    _frontFaceStencil = [[MTLStencilDescriptor alloc] init];
    _backFaceStencil = [[MTLStencilDescriptor alloc] init];
  }
  return self;
}

- (void)dealloc {
  [_frontFaceStencil release];
  [_backFaceStencil release];
  [super dealloc];
}
@end

// --- vertex layout ---------------------------------------------------------

@implementation MTLVertexAttributeDescriptor
@end

@implementation MTLVertexAttributeDescriptorArray
- (instancetype)init {
  return [super initWithElementClass:[MTLVertexAttributeDescriptor class]];
}
@end

@implementation MTLVertexBufferLayoutDescriptor
@end

@implementation MTLVertexBufferLayoutDescriptorArray
- (instancetype)init {
  return [super initWithElementClass:[MTLVertexBufferLayoutDescriptor class]];
}
@end

@implementation MTLVertexDescriptor {
  MTLVertexAttributeDescriptorArray *_attributes;
  MTLVertexBufferLayoutDescriptorArray *_layouts;
}

+ (MTLVertexDescriptor *)vertexDescriptor {
  return [[[self alloc] init] autorelease];
}

- (instancetype)init {
  if ((self = [super init])) {
    _attributes = [[MTLVertexAttributeDescriptorArray alloc] init];
    _layouts = [[MTLVertexBufferLayoutDescriptorArray alloc] init];
  }
  return self;
}

- (MTLVertexAttributeDescriptorArray *)attributes {
  return _attributes;
}
- (MTLVertexBufferLayoutDescriptorArray *)layouts {
  return _layouts;
}

- (void)reset {
  [_attributes release];
  [_layouts release];
  _attributes = [[MTLVertexAttributeDescriptorArray alloc] init];
  _layouts = [[MTLVertexBufferLayoutDescriptorArray alloc] init];
}

- (void)dealloc {
  [_attributes release];
  [_layouts release];
  [super dealloc];
}
@end
