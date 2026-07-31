#pragma once
#include "defines.h"

// Minimal NSWorkspace: WC3 uses it to open web links (openURL:), watch for system
// sleep via its notificationCenter, and look for other running copies of itself
// (runningApplicationsWithBundleIdentifier:). None of those exist here, so every
// answer is "no / nothing".
@interface NSWorkspace : NSObject
+ (NSWorkspace *)sharedWorkspace;

- (NSNotificationCenter *)notificationCenter;
- (BOOL)openURL:(NSURL *)url;
- (NSArray *)runningApplicationsWithBundleIdentifier:(NSString *)bundleIdentifier;
@end
