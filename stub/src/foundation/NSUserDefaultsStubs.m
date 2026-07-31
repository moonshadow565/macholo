#include <Foundation/NSString.h>
#include <Foundation/NSUserDefaults.h>

// GNUstep's NSUserDefaults has no suite (-initWithSuiteName:) support. WC3 opens a suite
// for its preferences; route it to the standard domain so reads/writes still work.
@interface NSUserDefaults (MacholoSuite)
@end
@implementation NSUserDefaults (MacholoSuite)
- (id)initWithSuiteName:(NSString *)suiteName {
  return [self init];
}
@end
