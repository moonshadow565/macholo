#include "NSWindowController.h"
#include <common.h>

@implementation NSWindowController {
  NSWindow *_window;
}

- (instancetype)init {
  [super init];
  _window = Nil;
  return self;
}

- (instancetype)initWithWindowNibName:(NSString *)windowNibName {
  // No nib loading; log so a surprise dialog path is visible in the run log.
  fprintf(stderr, "[NSWindowController] initWithWindowNibName: %s (not loaded)\n",
          windowNibName ? [windowNibName UTF8String] : "(null)");
  return [self init];
}

- (NSWindow *)window {
  return _window;
}

- (void)setWindow:(NSWindow *)window {
  _window = window;
}

- (void)showWindow:(id)sender {
  // Nothing to show -- the window was never loaded from a nib.
}

- (void)close {
}
@end
