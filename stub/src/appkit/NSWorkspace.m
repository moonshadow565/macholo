#include "NSWorkspace.h"
#include <common.h>

// DATA:
NSString *NSWorkspaceLaunchConfigurationArguments = @"NSWorkspaceLaunchConfigurationArguments";
NSString *NSWorkspaceWillSleepNotification = @"NSWorkspaceWillSleepNotification";

@implementation NSWorkspace

+ (NSWorkspace *)sharedWorkspace {
  static NSWorkspace *instance = Nil;
  if (instance == Nil) instance = [[NSWorkspace alloc] init];
  return instance;
}

- (NSNotificationCenter *)notificationCenter {
  // A private center would also work; the default center is enough because nobody
  // else posts the NSWorkspace* notification names the game observes.
  return [NSNotificationCenter defaultCenter];
}

- (BOOL)openURL:(NSURL *)url {
  // No browser to hand off to; log the URL instead of silently eating it.
  fprintf(stderr, "[NSWorkspace] openURL: %s (ignored)\n", url ? [[url absoluteString] UTF8String] : "(null)");
  return NO;
}

- (NSArray *)runningApplicationsWithBundleIdentifier:(NSString *)bundleIdentifier {
  // "No other copies of the game are running."
  return [NSArray array];
}
@end
