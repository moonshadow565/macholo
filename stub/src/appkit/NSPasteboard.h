#pragma once
#include "defines.h"

typedef NSString *NSPasteboardType;
typedef NSString *NSPasteboardName;

extern NSPasteboardType NSPasteboardTypeString;

// Minimal in-process pasteboard. There is no system pasteboard to talk to here,
// so this just keeps type -> string pairs in memory; that is enough for the
// game's copy/paste path (16.3 uses generalPasteboard/clearContents/
// setString:forType:/stringForType:/pasteboardItems).
@interface NSPasteboardItem : NSObject
- (NSString *)stringForType:(NSPasteboardType)type;
- (BOOL)setString:(NSString *)string forType:(NSPasteboardType)type;
- (NSArray<NSPasteboardType> *)types;
@end

@interface NSPasteboard : NSObject
+ (NSPasteboard *)generalPasteboard;
+ (NSPasteboard *)pasteboardWithName:(NSPasteboardName)name;

- (NSInteger)clearContents;
- (NSInteger)changeCount;
- (NSArray<NSPasteboardItem *> *)pasteboardItems;
- (NSArray<NSPasteboardType> *)types;

- (NSString *)stringForType:(NSPasteboardType)type;
- (BOOL)setString:(NSString *)string forType:(NSPasteboardType)type;
- (NSInteger)declareTypes:(NSArray<NSPasteboardType> *)types owner:(id)owner;
@end
