#include "NSNib.h"
#include <common.h>

@implementation NSNib
- (id)initWithContentsOfURL:(NSURL *)nibFileURL {
  return [self init];
}
- (instancetype)initWithNibNamed:(NSString *)nibNamed bundle:(NSBundle *)bundle {
  return [self init];
}
- (instancetype)initWithNibData:(NSData *)nibData bundle:(NSBundle *)bundle {
  return [self init];
}

- (BOOL)instantiateNibWithOwner:(id)owner topLevelObjects:(NSArray *_Null_unspecified *)topLevelObjects {
  return YES;
}
- (BOOL)instantiateNibWithExternalNameTable:(NSDictionary *)externalNameTable {
  return YES;
}
- (BOOL)instantiateWithOwner:(id)owner topLevelObjects:(NSArray *_Nullable *)topLevelObjects {
  return YES;
}
@end
