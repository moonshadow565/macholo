#pragma once
// The 7 MTL*Descriptor classes the 16.15 binary imports, plus the sub-descriptors and
// sub-descriptor arrays they hand out. All of them are plain property bags: nothing here
// validates anything, and nothing here is read by the null renderer except the texture
// descriptor's dimensions.
//
// Only properties the generic fallback cannot carry (structs, floats, lazily-created
// sub-objects, retained objects) are declared by hand; every other property the engine
// touches is synthesized on demand and logged. See MACHOLONullObject.h.
#import "MACHOLONullObject.h"

// Descriptors are NSCopying on real Metal and engines do copy them. They are inert here,
// so a copy is the same object.
@interface MACHOLODescriptor : MACHOLONullObject <NSCopying>
@end

// Lazily-populated MTL*DescriptorArray: objectAtIndexedSubscript: creates the element on
// first touch, which is how the real ones behave for attachment/layout arrays.
@interface MACHOLODescriptorArray : MACHOLONullObject
- (instancetype)initWithElementClass:(Class)elementClass;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
- (void)setObject:(id)obj atIndexedSubscript:(NSUInteger)index;
@end

// --- render pass -----------------------------------------------------------

@interface MTLRenderPassAttachmentDescriptor : MACHOLODescriptor
@property(nonatomic, retain) id texture;
@property(nonatomic, retain) id resolveTexture;
@end

@interface MTLRenderPassColorAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
@property(nonatomic, assign) MTLClearColor clearColor;
@end

@interface MTLRenderPassDepthAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
@property(nonatomic, assign) double clearDepth;
@end

@interface MTLRenderPassStencilAttachmentDescriptor : MTLRenderPassAttachmentDescriptor
@property(nonatomic, assign) uint32_t clearStencil;
@end

@interface MTLRenderPassColorAttachmentDescriptorArray : MACHOLODescriptorArray
@end

@interface MTLRenderPassDescriptor : MACHOLODescriptor
+ (MTLRenderPassDescriptor *)renderPassDescriptor;
@property(nonatomic, readonly) MTLRenderPassColorAttachmentDescriptorArray *colorAttachments;
@property(nonatomic, retain) MTLRenderPassDepthAttachmentDescriptor *depthAttachment;
@property(nonatomic, retain) MTLRenderPassStencilAttachmentDescriptor *stencilAttachment;
@end

// --- render pipeline -------------------------------------------------------

@interface MTLRenderPipelineColorAttachmentDescriptor : MACHOLODescriptor
@end

@interface MTLRenderPipelineColorAttachmentDescriptorArray : MACHOLODescriptorArray
@end

@interface MTLRenderPipelineDescriptor : MACHOLODescriptor
@property(nonatomic, retain) id vertexFunction;
@property(nonatomic, retain) id fragmentFunction;
@property(nonatomic, retain) id vertexDescriptor;
@property(nonatomic, readonly) MTLRenderPipelineColorAttachmentDescriptorArray *colorAttachments;
- (void)reset;
@end

// --- textures / samplers ---------------------------------------------------

@interface MTLTextureDescriptor : MACHOLODescriptor
+ (MTLTextureDescriptor *)texture2DDescriptorWithPixelFormat:(MTLPixelFormat)pixelFormat
                                                       width:(NSUInteger)width
                                                      height:(NSUInteger)height
                                                   mipmapped:(BOOL)mipmapped;
+ (MTLTextureDescriptor *)textureCubeDescriptorWithPixelFormat:(MTLPixelFormat)pixelFormat
                                                          size:(NSUInteger)size
                                                     mipmapped:(BOOL)mipmapped;
+ (MTLTextureDescriptor *)textureBufferDescriptorWithPixelFormat:(MTLPixelFormat)pixelFormat
                                                           width:(NSUInteger)width
                                                 resourceOptions:(MTLResourceOptions)options
                                                           usage:(MTLTextureUsage)usage;
@property(nonatomic, assign) MTLTextureType textureType;
@property(nonatomic, assign) MTLPixelFormat pixelFormat;
@property(nonatomic, assign) NSUInteger width;
@property(nonatomic, assign) NSUInteger height;
@property(nonatomic, assign) NSUInteger depth;
@property(nonatomic, assign) NSUInteger mipmapLevelCount;
@property(nonatomic, assign) NSUInteger sampleCount;
@property(nonatomic, assign) NSUInteger arrayLength;
@property(nonatomic, assign) MTLResourceOptions resourceOptions;
@property(nonatomic, assign) MTLCPUCacheMode cpuCacheMode;
@property(nonatomic, assign) MTLStorageMode storageMode;
@property(nonatomic, assign) MTLTextureUsage usage;
@end

@interface MTLSamplerDescriptor : MACHOLODescriptor
// float, so the fallback (which only moves integer registers) cannot carry these.
@property(nonatomic, assign) float lodMinClamp;
@property(nonatomic, assign) float lodMaxClamp;
@end

// --- depth/stencil ---------------------------------------------------------

@interface MTLStencilDescriptor : MACHOLODescriptor
@end

@interface MTLDepthStencilDescriptor : MACHOLODescriptor
// Default-constructed on a real device, not nil.
@property(nonatomic, retain) MTLStencilDescriptor *frontFaceStencil;
@property(nonatomic, retain) MTLStencilDescriptor *backFaceStencil;
@end

// --- vertex layout ---------------------------------------------------------

@interface MTLVertexAttributeDescriptor : MACHOLODescriptor
@end

@interface MTLVertexAttributeDescriptorArray : MACHOLODescriptorArray
@end

@interface MTLVertexBufferLayoutDescriptor : MACHOLODescriptor
@end

@interface MTLVertexBufferLayoutDescriptorArray : MACHOLODescriptorArray
@end

@interface MTLVertexDescriptor : MACHOLODescriptor
+ (MTLVertexDescriptor *)vertexDescriptor;
@property(nonatomic, readonly) MTLVertexAttributeDescriptorArray *attributes;
@property(nonatomic, readonly) MTLVertexBufferLayoutDescriptorArray *layouts;
- (void)reset;
@end
