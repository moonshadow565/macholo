#pragma once
#include "defines.h"

@interface NSOpenGLContext : NSObject
- (instancetype)initWithFormat:(NSOpenGLPixelFormat *)format shareContext:(NSOpenGLContext *)share;

+ (void)clearCurrentContext;
+ (NSOpenGLContext *)currentContext;
- (void)makeCurrentContext;

- (NSView *)view;
- (void)setView:(NSView *)view;
- (void)update;
- (void)flushBuffer;

- (void)getValues:(GLint *)vals forParameter:(NSOpenGLContextParameter)parameter;
- (void)setValues:(const GLint *)vals forParameter:(NSOpenGLContextParameter)param;
@end

@interface NSOpenGLPixelFormat : NSObject
- (id)initWithData:(NSData *)attribs;
- (instancetype)initWithAttributes:(const NSOpenGLPixelFormatAttribute *)attribs;
- (NSData *)attributes;
- (void)setAttributes:(NSData *)attribs;
@end
