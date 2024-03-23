#pragma once
#include "defines.h"

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
@end
