#pragma once
#include "defines.h"
#include <SDL2/SDL_events.h>

@interface NSEvent : NSObject <NSCopying>
- (instancetype)initFromSDL:(void *)sdl_event;

- (NSEventType)type;
- (NSEventSubtype)subtype;

- (NSPoint)locationInWindow;
- (NSTimeInterval)timestamp;
- (NSWindow *)window;
- (void*) sdlWindow;
- (NSInteger)windowNumber;

+ (NSEventModifierFlags)modifierFlags;
- (NSEventModifierFlags)modifierFlags;
- (NSString *)characters;
- (unsigned short)keyCode;
- (BOOL)isARepeat;

- (NSInteger)buttonNumber;
- (CGFloat)deltaX;
- (CGFloat)deltaY;

+ (NSPoint)mouseLocation;
@end
