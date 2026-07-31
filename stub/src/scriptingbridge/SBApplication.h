#pragma once
#include "defines.h"

// ScriptingBridge stub. There is nothing to script here: +applicationWithBundleIdentifier:
// answers nil ("that application is not available") and the class is otherwise inert.
@interface SBApplication : NSObject
+ (SBApplication *)applicationWithBundleIdentifier:(NSString *)ident;
@end
