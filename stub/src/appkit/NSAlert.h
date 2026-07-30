#pragma once
#include "defines.h"

enum {
  NSAlertFirstButtonReturn = 1000,
  NSAlertSecondButtonReturn = 1001,
  NSAlertThirdButtonReturn = 1002,
};

@protocol NSAlertDelegate <NSObject>
@end

@interface NSAlert : NSObject
- (NSString *)informativeText;
- (void)setInformativeText:(NSString *)value;
- (NSString *)messageText;
- (void)setMessageText:(NSString *)value;

- (NSImage *)icon;
- (void)setIcon:(NSImage *)value;

- (NSButton *)addButtonWithTitle:(NSString *)title;

- (NSInteger)runModal;
- (void)beginSheetModalForWindow:(NSWindow *)sheetWindow completionHandler:(void (^)(NSInteger))handler;
@end
