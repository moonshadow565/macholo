#include "NSPasteboard.h"
#include <common.h>

NSPasteboardType NSPasteboardTypeString = @"public.utf8-plain-text";

@implementation NSPasteboardItem {
  NSMutableDictionary<NSPasteboardType, NSString *> *_contents;
}

- (instancetype)init {
  [super init];
  _contents = [[NSMutableDictionary alloc] init];
  return self;
}

- (NSString *)stringForType:(NSPasteboardType)type {
  if (type == Nil) return Nil;
  return [_contents objectForKey:type];
}

- (BOOL)setString:(NSString *)string forType:(NSPasteboardType)type {
  if (type == Nil) return NO;
  if (string == Nil)
    [_contents removeObjectForKey:type];
  else
    [_contents setObject:[[string copy] autorelease] forKey:type];
  return YES;
}

- (NSArray<NSPasteboardType> *)types {
  return [_contents allKeys];
}

- (void)dealloc {
  [_contents release];
  [super dealloc];
}
@end

@implementation NSPasteboard {
  NSPasteboardItem *_item;
  NSInteger _changeCount;
}

- (instancetype)init {
  [super init];
  _item = [[NSPasteboardItem alloc] init];
  _changeCount = 0;
  return self;
}

+ (NSPasteboard *)generalPasteboard {
  static NSPasteboard *instance = Nil;
  if (instance == Nil) instance = [[NSPasteboard alloc] init];
  return instance;
}

+ (NSPasteboard *)pasteboardWithName:(NSPasteboardName)name {
  // Only one board is modelled; the game never juggles several.
  return [NSPasteboard generalPasteboard];
}

- (NSInteger)clearContents {
  [_item release];
  _item = [[NSPasteboardItem alloc] init];
  return ++_changeCount;
}

- (NSInteger)changeCount {
  return _changeCount;
}

- (NSArray<NSPasteboardItem *> *)pasteboardItems {
  return @[ _item ];
}

- (NSArray<NSPasteboardType> *)types {
  return [_item types];
}

- (NSString *)stringForType:(NSPasteboardType)type {
  return [_item stringForType:type];
}

- (BOOL)setString:(NSString *)string forType:(NSPasteboardType)type {
  BOOL ok = [_item setString:string forType:type];
  if (ok) ++_changeCount;
  return ok;
}

- (NSInteger)declareTypes:(NSArray<NSPasteboardType> *)types owner:(id)owner {
  return [self clearContents];
}
@end
