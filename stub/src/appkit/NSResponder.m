#include "NSResponder.h"
#include <common.h>
#include "input_debug.h"

@implementation NSResponder {
  NSResponder *nextResponder_;
}

- (NSResponder *)nextResponder {
  return nextResponder_;
}
- (void)setNextResponder:(NSResponder *)responder {
  INPUT_DEBUG("setNextResponder: %s -> %s", class_getName([self class]),
              responder ? class_getName([responder class]) : "(nil)");
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
- (void)noResponderFor:(SEL)action {
  INPUT_DEBUG("noResponderFor: %s (chain ended at %s)", sel_getName(action), class_getName([self class]));
}

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
