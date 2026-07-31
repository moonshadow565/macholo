#include "SBApplication.h"
#include <common.h>

@implementation SBApplication

+ (SBApplication *)applicationWithBundleIdentifier:(NSString *)ident {
  fprintf(stderr, "[SBApplication] applicationWithBundleIdentifier: %s -> nil\n",
          ident ? [ident UTF8String] : "(null)");
  return Nil;
}
@end
