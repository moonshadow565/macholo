#include "NSResponder.h"
#include <common.h>

@implementation NSResponder {
  NSResponder *nextResponder_;
}

- (NSResponder *)nextResponder {
  return nextResponder_;
}
- (void)setNextResponder:(NSResponder *)responder {
  nextResponder_ = responder;
}

- (BOOL)acceptsFirstResponder {
  return NO;
}
- (BOOL)becomeFirstResponder {
  return YES;
}
- (BOOL)resignFirstResponder {
  return YES;
}
- (void)noResponderFor:(SEL)action {}

#define HANDLE($name)                          \
  -(void)$name : (NSEvent *)event {            \
    if (nextResponder_)                        \
      [nextResponder_ $name:event];            \
    else                                       \
      [self noResponderFor:@selector($name:)]; \
  }
HANDLE(mouseDown);
HANDLE(mouseDragged);
HANDLE(mouseUp);
HANDLE(mouseMoved);
HANDLE(mouseEntered);
HANDLE(mouseExited);
HANDLE(rightMouseDown);
HANDLE(rightMouseUp);
HANDLE(rightMouseDragged);
HANDLE(otherMouseDown);
HANDLE(otherMouseUp);
HANDLE(otherMouseDragged);
HANDLE(keyDown);
HANDLE(keyUp);
HANDLE(cursorUpdate);
HANDLE(flagsChanged);
HANDLE(helpRequested);
HANDLE(scrollWheel);
HANDLE(quickLookWithEvent);
HANDLE(changeModeWithEvent);
#undef HANDLE

@end
