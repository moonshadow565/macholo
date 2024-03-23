#pragma once
#include "NSView.h"

@interface NSNib : NSObject
- (id)initWithContentsOfURL:(NSURL *)nibFileURL;
- (instancetype)initWithNibNamed:(NSString *)nibNamed bundle:(NSBundle *)bundle;
- (instancetype)initWithNibData:(NSData *)nibData bundle:(NSBundle *)bundle;

- (BOOL)instantiateNibWithOwner:(id)owner topLevelObjects:(NSArray *_Null_unspecified *)topLevelObjects;
- (BOOL)instantiateNibWithExternalNameTable:(NSDictionary *)externalNameTable;
- (BOOL)instantiateWithOwner:(id)owner topLevelObjects:(NSArray *_Nullable *)topLevelObjects;
@end
