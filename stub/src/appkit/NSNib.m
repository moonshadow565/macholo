#include "NSNib.h"
#include <common.h>
#include <Foundation/NSArray.h>
#include <Foundation/NSBundle.h>

// -[NSBundle loadNibNamed:owner:topLevelObjects:] is the AppKit NSNibLoading category, not
// present in this GNUstep build (and the nibs are Apple's compiled NIBArchive format,
// which GNUstep cannot instantiate anyway). WC3 loads "Application" with owner=NSApp for
// its (irrelevant under Xvfb) menu bar; without this method it hits doesNotRecognizeSelector
// and aborts. Acknowledge the load so the game proceeds; hand back an empty top-level list.
@interface NSBundle (MacholoNibLoading)
@end
@implementation NSBundle (MacholoNibLoading)
- (BOOL)loadNibNamed:(NSString *)name owner:(id)owner topLevelObjects:(NSArray **)topLevelObjects {
  fprintf(stderr, "[NSBundle] loadNibNamed:%s owner:%s -> stubbed\n", name ? [name UTF8String] : "(null)",
          owner ? object_getClassName(owner) : "(nil)");
  fflush(stderr);
  if (topLevelObjects) *topLevelObjects = @[];
  return YES;
}
@end

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
