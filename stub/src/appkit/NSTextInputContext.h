#pragma once
#include "defines.h"

@protocol NSTextInputClient
@required
- (BOOL)hasMarkedText;
- (NSRange)markedRange;
- (NSRange)selectedRange;
- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange;
- (void)unmarkText;
- (NSArray *)validAttributesForMarkedText;

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange;
- (void)insertText:(id)string replacementRange:(NSRange)replacementRange;

- (NSUInteger)characterIndexForPoint:(NSPoint)point;
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange;

- (void)doCommandBySelector:(SEL)selector;

@optional
- (NSAttributedString *)attributedString;
- (CGFloat)fractionOfDistanceThroughGlyphForPoint:(NSPoint)point;
- (CGFloat)baselineDeltaForCharacterAtIndex:(NSUInteger)anIndex;
- (NSInteger)windowLevel;
- (BOOL)drawsVerticallyForCharacterAtIndex:(NSUInteger)charIndex;

@end

@interface NSTextInputContext : NSObject
- (instancetype)initWithClient:(id<NSTextInputClient>)client;

- (id<NSTextInputClient>) client;

- (BOOL)handleEvent:(NSEvent *)event;
@end
