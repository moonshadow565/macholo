#pragma once
#include "defines.h"

@class NSWindow;

// Minimal NSWindowController: WC3's CDKeyDialogController subclasses this. The dialog
// only appears when no CD key is stored, and there is no nib loading here anyway, so
// the controller just holds a (nil) window and swallows showWindow:.
@interface NSWindowController : NSObject
- (instancetype)init;
- (instancetype)initWithWindowNibName:(NSString *)windowNibName;

- (NSWindow *)window;
- (void)setWindow:(NSWindow *)window;

- (void)showWindow:(id)sender;
- (void)close;
@end
