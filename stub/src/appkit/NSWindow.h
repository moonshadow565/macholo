#pragma once
#include "NSResponder.h"
#include "defines.h"

@class NSView;
@class NSImage;

@interface NSWindow : NSResponder
- (void *)getSDLWindow;
+ (NSWindow*)fromSDLWindowNumber: (NSUInteger)num;
- (void)handleSDLEvent: (void*)sdl_event;

- (instancetype)initWithContentRect:(NSRect)contentRect
                          styleMask:(NSWindowStyleMask)style
                            backing:(NSBackingStoreType)backingStoreType
                              defer:(BOOL)flag;

- (id)delegate;
- (void)setDelegate:(id)delegate;

- (NSView *)contentView;
- (void)setContentView:(NSView *)view;

- (NSWindowStyleMask)styleMask;
- (void)setStyleMask:(NSWindowStyleMask)styleMask;

- (BOOL)hidesOnDeactivate;
- (void)setHidesOnDeactivate:(BOOL)flag;

- (BOOL)isOpaque;
- (void)setOpaque:(BOOL)value;

- (NSRect)contentRectForFrameRect:(NSRect)rect;
- (NSRect)frameRectForContentRect:(NSRect)rect;

// Frame for window is in screen cordinates, starts at bottom left.
- (NSRect)frame;
- (void)setFrame:(NSRect)frame display:(BOOL)display;

- (void)orderOut:(id)sender;
- (void)orderBack:(id)sender;
- (void)orderFront:(id)sender;

- (NSWindowLevel)level;
- (void)setLevel:(NSWindowLevel)value;

- (BOOL)isVisible;

- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (BOOL)isKeyWindow;
- (void)makeKeyAndOrderFront:(id)sender;

- (NSEvent *)currentEvent;
- (NSEvent *)nextEventMatchingMask:(NSEventMask)mask
                         untilDate:(NSDate *)untilDate
                            inMode:(NSString *)mode
                           dequeue:(BOOL)dequeue;
- (void)sendEvent:(NSEvent *)event;

- (void)keyDown:(NSEvent *)event;
- (void)keyUp:(NSEvent *)event;

- (NSView *)initialFirstResponder;
- (void)setInitialFirstResponder:(NSView *)view;

- (NSResponder *)firstResponder;
- (BOOL)makeFirstResponder:(NSResponder *)responder;

- (BOOL)acceptsMouseMovedEvents;
- (void)setAcceptsMouseMovedEvents:(BOOL)flag;

- (BOOL)ignoresMouseEvents;
- (void)setIgnoresMouseEvents:(BOOL)value;

- (void)display;
- (void)update;

- (NSRect)convertRectToScreen:(NSRect)rect;

- (NSString *)title;
- (void)setTitle:(NSString *)title;

- (NSScreen *)screen;

- (void)close;

- (BOOL)isReleasedWhenClosed;
- (void)setReleasedWhenClosed:(BOOL)flag;

- (void)miniaturize:(id)sender;
- (void)deminiaturize:(id)sender;
@end

@protocol NSWindowDelegate <NSObject>
@optional
- (BOOL)window:(NSWindow *)window
    shouldDragDocumentWithEvent:(NSEvent *)event
                           from:(NSPoint)dragImageLocation
                 withPasteboard:(NSPasteboard *)pasteboard;
- (BOOL)window:(NSWindow *)window shouldPopUpDocumentPathMenu:(NSMenu *)menu;
- (BOOL)windowShouldClose:(id)sender;
- (BOOL)windowShouldZoom:(NSWindow *)window toFrame:(NSRect)frame;
- (id)windowWillReturnFieldEditor:(NSWindow *)window toObject:(id)client;
- (NSApplicationPresentationOptions)window:(NSWindow *)window
      willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions;
- (NSArray<NSWindow *> *)customWindowsToEnterFullScreenForWindow:(NSWindow *)window onScreen:(NSScreen *)screen;
- (NSArray<NSWindow *> *)customWindowsToEnterFullScreenForWindow:(NSWindow *)window;
- (NSArray<NSWindow *> *)customWindowsToExitFullScreenForWindow:(NSWindow *)window;
- (NSRect)window:(NSWindow *)window willPositionSheet:(NSWindow *)sheet usingRect:(NSRect)rect;
- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)frame;
- (NSSize)window:(NSWindow *)window
    willResizeForVersionBrowserWithMaxPreferredSize:(NSSize)maxPreferredFrameSize
                                     maxAllowedSize:(NSSize)maxAllowedFrameSize;
- (NSSize)window:(NSWindow *)window willUseFullScreenContentSize:(NSSize)proposedSize;
- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize;
- (NSUndoManager *)windowWillReturnUndoManager:(NSWindow *)window;
- (void)window:(NSWindow *)window didDecodeRestorableState:(NSCoder *)state;
- (void)window:(NSWindow *)window
    startCustomAnimationToEnterFullScreenOnScreen:(NSScreen *)screen
                                     withDuration:(NSTimeInterval)duration;
- (void)window:(NSWindow *)window startCustomAnimationToEnterFullScreenWithDuration:(NSTimeInterval)duration;
- (void)window:(NSWindow *)window startCustomAnimationToExitFullScreenWithDuration:(NSTimeInterval)duration;
- (void)window:(NSWindow *)window willEncodeRestorableState:(NSCoder *)state;
- (void)windowDidFailToEnterFullScreen:(NSWindow *)window;
- (void)windowDidFailToExitFullScreen:(NSWindow *)window;

- (void)windowDidBecomeKey:(NSNotification *)notification;
- (void)windowDidBecomeMain:(NSNotification *)notification;
- (void)windowDidChangeBackingProperties:(NSNotification *)notification;
- (void)windowDidChangeOcclusionState:(NSNotification *)notification;
- (void)windowDidChangeScreen:(NSNotification *)notification;
- (void)windowDidChangeScreenProfile:(NSNotification *)notification;
- (void)windowDidDeminiaturize:(NSNotification *)notification;
- (void)windowDidEndLiveResize:(NSNotification *)notification;
- (void)windowDidEndSheet:(NSNotification *)notification;
- (void)windowDidEnterFullScreen:(NSNotification *)notification;
- (void)windowDidEnterVersionBrowser:(NSNotification *)notification;
- (void)windowDidExitFullScreen:(NSNotification *)notification;
- (void)windowDidExitVersionBrowser:(NSNotification *)notification;
- (void)windowDidExpose:(NSNotification *)notification;
- (void)windowDidMiniaturize:(NSNotification *)notification;
- (void)windowDidMove:(NSNotification *)notification;
- (void)windowDidResignKey:(NSNotification *)notification;
- (void)windowDidResignMain:(NSNotification *)notification;
- (void)windowDidResize:(NSNotification *)notification;
- (void)windowDidUpdate:(NSNotification *)notification;
- (void)windowWillBeginSheet:(NSNotification *)notification;
- (void)windowWillClose:(NSNotification *)notification;
- (void)windowWillEnterFullScreen:(NSNotification *)notification;
- (void)windowWillEnterVersionBrowser:(NSNotification *)notification;
- (void)windowWillExitFullScreen:(NSNotification *)notification;
- (void)windowWillExitVersionBrowser:(NSNotification *)notification;
- (void)windowWillMiniaturize:(NSNotification *)notification;
- (void)windowWillMove:(NSNotification *)notification;
- (void)windowWillStartLiveResize:(NSNotification *)notification;
@end
