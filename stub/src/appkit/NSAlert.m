#include "NSAlert.h"
#include <common.h>

// NSAlert only ever shows up on real error paths, so panicking here hides the actual
// error message. Store the properties, log them to stderr, and report the default
// button (NSAlertFirstButtonReturn) from runModal so the client continues down its
// normal path instead of trapping.
@implementation NSAlert {
  NSString *_messageText;
  NSString *_informativeText;
  NSImage *_icon;
  long _buttonCount;
}

- (NSString *)informativeText {
  return _informativeText;
}
- (void)setInformativeText:(NSString *)value {
  _informativeText = [value retain];
}
- (NSString *)messageText {
  return _messageText;
}
- (void)setMessageText:(NSString *)value {
  _messageText = [value retain];
}

- (NSImage *)icon {
  return _icon;
}
- (void)setIcon:(NSImage *)value {
  _icon = [value retain];
}

- (NSButton *)addButtonWithTitle:(NSString *)title {
  fprintf(stderr, "[NSAlert] button %ld: %s\n", _buttonCount++, title ? [title UTF8String] : "(null)");
  return NULL;
}

- (void)_logToStderr {
  fprintf(stderr, "[NSAlert] messageText: %s\n", _messageText ? [_messageText UTF8String] : "(none)");
  fprintf(stderr, "[NSAlert] informativeText: %s\n", _informativeText ? [_informativeText UTF8String] : "(none)");
  fflush(stderr);
}

- (NSInteger)runModal {
  [self _logToStderr];
  return NSAlertFirstButtonReturn;
}

- (void)beginSheetModalForWindow:(NSWindow *)sheetWindow completionHandler:(void (^)(NSInteger))handler {
  [self _logToStderr];
  if (handler) handler(NSAlertFirstButtonReturn);
}
@end
