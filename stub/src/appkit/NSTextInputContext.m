#include "NSTextInputContext.h"
#include "NSEvent.h"
#include "enums.h"

@implementation  NSTextInputContext {
  id<NSTextInputClient> _client;
  NSUInteger _count;
}

- (instancetype)initWithClient:(id<NSTextInputClient>)client {
    _client = client;
    return self;
}

- (id<NSTextInputClient>) client {
    return _client;
}

- (BOOL)handleEvent:(NSEvent *)event {
    if ([event type] != NSEventTypeKeyDown) {
        return NO;
    }
    if ([event keyCode] == 36) {
        // enter
        return YES;
    }
    // Provides info:
    // attributedString
    // attributedSubstringForProposedRange
    // characterIndexForPoint
    // firstRectForCharacterRange
    // markedRange
    // selectedRange

    // cancelOperation
    // deleteBackward
    // deleteForward
    // hasMarkedText
    // initWithFrame
    // insertNewline
    // insertTab
    // insertText
    // keyDown
    // moveDown
    // moveDownAndModifySelection
    // moveLeft
    // moveLeftAndModifySelection
    // moveRight
    // moveRightAndModifySelection
    // moveToBeginningOfDocumentAndModifySelection
    // moveToBeginningOfLine
    // moveToBeginningOfLineAndModifySelection
    // moveToEndOfDocumentAndModifySelection
    // moveToEndOfLine
    // moveToEndOfLineAndModifySelection
    // moveToLeftEndOfLine
    // moveToLeftEndOfLineAndModifySelection
    // moveToRightEndOfLine
    // moveToRightEndOfLineAndModifySelection
    // moveUp
    // moveUpAndModifySelection
    // scrollToBeginningOfDocument
    // scrollToEndOfDocument
    // setMarkedText
    // unmarkText
    // validAttributesForMarkedText

    // FIXME: handle all other events here here?
    NSString* characters = [event characters];
    if (characters == Nil || [characters length] != 1) {
        return YES;
    }
    // NOTE: -attributedString is optional in NSTextInputClient and the game's
    // IMETextInput does not implement it (TextInputClientMacOSX does), so asking
    // for it unconditionally would abort on an unrecognized selector.
    if (![_client respondsToSelector: @selector(insertText:replacementRange:)]) {
        return YES;
    }
    [_client insertText: characters replacementRange: (NSRange){0, 0}];
    _count += [characters length];
    return YES;
}
@end
