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
  id _accessoryView;
}

// The alert is only ever logged and dismissed (runModal returns the default button), so
// the accessory view / window / layout plumbing just needs to exist: without these the
// game hits doesNotRecognizeSelector, and its own @catch handler then walks the
// exception's callStackReturnAddresses -- which GNUstep hands back as GSPointerValue
// (^v) NSValues rather than Apple's NSNumbers -- calling -unsignedIntegerValue on each
// and raising a second, uncaught exception that aborts the process.
- (id)accessoryView {
  return _accessoryView;
}
- (void)setAccessoryView:(id)value {
  _accessoryView = [value retain];
}
- (void)layout {
}
- (id)window {
  return nil;
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
