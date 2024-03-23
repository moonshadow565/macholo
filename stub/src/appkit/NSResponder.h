#pragma once
#include "defines.h"

@interface NSResponder : NSObject
- (NSResponder *)nextResponder;
- (void)setNextResponder:(NSResponder *)responder;

- (BOOL)acceptsFirstResponder;
- (BOOL)becomeFirstResponder;
- (BOOL)resignFirstResponder;

- (void)noResponderFor:(SEL)action;

- (void)mouseDown:(NSEvent *)event;
- (void)mouseDragged:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)mouseMoved:(NSEvent *)event;
- (void)mouseEntered:(NSEvent *)event;
- (void)mouseExited:(NSEvent *)event;

- (void)rightMouseDown:(NSEvent *)event;
- (void)rightMouseUp:(NSEvent *)event;
- (void)rightMouseDragged:(NSEvent *)event;

- (void)otherMouseDown:(NSEvent *)event;
- (void)otherMouseUp:(NSEvent *)event;
- (void)otherMouseDragged:(NSEvent *)event;

- (void)keyDown:(NSEvent *)event;
- (void)keyUp:(NSEvent *)event;

- (void)cursorUpdate:(NSEvent *)event;
- (void)flagsChanged:(NSEvent *)event;
- (void)helpRequested:(NSEvent *)eventPtr;
- (void)scrollWheel:(NSEvent *)event;
- (void)quickLookWithEvent:(NSEvent *)event;
- (void)changeModeWithEvent:(NSEvent *)event;
@end

@interface NSResponder (NSStandardKeyBindingResponding)
- (void)doCommandBySelector:(SEL)selector;

- (void)insertBacktab:(id)sender;
- (void)insertContainerBreak:(id)sender;
- (void)insertDoubleQuoteIgnoringSubstitution:(id)sender;
- (void)insertLineBreak:(id)sender;
- (void)insertNewline:(id)sender;
- (void)insertNewlineIgnoringFieldEditor:(id)sender;
- (void)insertParagraphSeparator:(id)sender;
- (void)insertSingleQuoteIgnoringSubstitution:(id)sender;
- (void)insertTab:(id)sender;
- (void)insertTabIgnoringFieldEditor:(id)sender;
- (void)insertText:(NSString *)text;

- (void)deleteBackward:(id)sender;
- (void)deleteBackwardByDecomposingPreviousCharacter:(id)sender;
- (void)deleteForward:(id)sender;
- (void)deleteToBeginningOfLine:(id)sender;
- (void)deleteToBeginningOfParagraph:(id)sender;
- (void)deleteToEndOfLine:(id)sender;
- (void)deleteToEndOfParagraph:(id)sender;
- (void)deleteWordBackward:(id)sender;
- (void)deleteWordForward:(id)sender;
- (void)yank:(id)sender;

- (void)moveBackward:(id)sender;
- (void)moveDown:(id)sender;
- (void)moveForward:(id)sender;
- (void)moveLeft:(id)sender;
- (void)moveRight:(id)sender;
- (void)moveUp:(id)sender;

- (void)moveBackwardAndModifySelection:(id)sender;
- (void)moveDownAndModifySelection:(id)sender;
- (void)moveForwardAndModifySelection:(id)sender;
- (void)moveLeftAndModifySelection:(id)sender;
- (void)moveRightAndModifySelection:(id)sender;
- (void)moveUpAndModifySelection:(id)sender;

- (void)scrollPageDown:(id)sender;
- (void)scrollPageUp:(id)sender;
- (void)scrollLineDown:(id)sender;
- (void)scrollLineUp:(id)sender;
- (void)scrollToBeginningOfDocument:(id)sender;
- (void)scrollToEndOfDocument:(id)sender;
- (void)pageDown:(id)sender;
- (void)pageUp:(id)sender;
- (void)pageUpAndModifySelection:(id)sender;
- (void)centerSelectionInVisibleArea:(id)sender;

- (void)transpose:(id)sender;
- (void)transposeWords:(id)sender;

- (void)indent:(id)sender;

- (void)cancelOperation:(id)sender;

- (void)quickLookPreviewItems:(id)sender;

- (void)makeBaseWritingDirectionLeftToRight:(id)sender;
- (void)makeBaseWritingDirectionNatural:(id)sender;
- (void)makeBaseWritingDirectionRightToLeft:(id)sender;
- (void)makeTextWritingDirectionLeftToRight:(id)sender;
- (void)makeTextWritingDirectionNatural:(id)sender;
- (void)makeTextWritingDirectionRightToLeft:(id)sender;

- (void)capitalizeWord:(id)sender;
- (void)changeCaseOfLetter:(id)sender;
- (void)lowercaseWord:(id)sender;
- (void)uppercaseWord:(id)sender;

- (void)moveToBeginningOfDocument:(id)sender;
- (void)moveToBeginningOfDocumentAndModifySelection:(id)sender;
- (void)moveToEndOfDocument:(id)sender;
- (void)moveToEndOfDocumentAndModifySelection:(id)sender;

- (void)moveParagraphBackwardAndModifySelection:(id)sender;
- (void)moveParagraphForwardAndModifySelection:(id)sender;
- (void)moveToBeginningOfParagraph:(id)sender;
- (void)moveToBeginningOfParagraphAndModifySelection:(id)sender;
- (void)moveToEndOfParagraph:(id)sender;
- (void)moveToEndOfParagraphAndModifySelection:(id)sender;

- (void)moveToBeginningOfLine:(id)sender;
- (void)moveToBeginningOfLineAndModifySelection:(id)sender;
- (void)moveToEndOfLine:(id)sender;
- (void)moveToEndOfLineAndModifySelection:(id)sender;
- (void)moveToLeftEndOfLine:(id)sender;
- (void)moveToLeftEndOfLineAndModifySelection:(id)sender;
- (void)moveToRightEndOfLine:(id)sender;
- (void)moveToRightEndOfLineAndModifySelection:(id)sender;

- (void)selectAll:(id)sender;
- (void)selectLine:(id)sender;
- (void)selectParagraph:(id)sender;
- (void)selectWord:(id)sender;

- (void)setMark:(id)sender;
- (void)selectToMark:(id)sender;
- (void)deleteToMark:(id)sender;
- (void)swapWithMark:(id)sender;

- (void)complete:(id)sender;

- (void)moveWordBackward:(id)sender;
- (void)moveWordBackwardAndModifySelection:(id)sender;
- (void)moveWordForward:(id)sender;
- (void)moveWordForwardAndModifySelection:(id)sender;
- (void)moveWordLeft:(id)sender;
- (void)moveWordLeftAndModifySelection:(id)sender;
- (void)moveWordRight:(id)sender;
- (void)moveWordRightAndModifySelection:(id)sender;

@end