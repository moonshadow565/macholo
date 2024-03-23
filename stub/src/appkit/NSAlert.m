#include "NSAlert.h"
#include <common.h>

// TODO: this should be easy to stub as pretty much everything is just a property.

@implementation NSAlert
- (NSString *)informativeText { panic("Unimplemented"); }
- (void)setInformativeText:(NSString *)value { panic("Unimplemented"); }
- (NSString *)messageText { panic("Unimplemented"); }
- (void)setMessageText:(NSString *)value { panic("Unimplemented"); }

- (NSImage *)icon { panic("Unimplemented"); }
- (void)setIcon:(NSImage *)value { panic("Unimplemented"); }

- (NSButton *)addButtonWithTitle:(NSString *)title { panic("Unimplemented"); }
@end
