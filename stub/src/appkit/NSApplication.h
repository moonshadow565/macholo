#pragma once
#include "NSEvent.h"
#include "NSResponder.h"
#include "defines.h"

@interface NSApplication : NSResponder
+ (NSApplication *)sharedApplication;

- init;

- (id)delegate;
- (void)setDelegate: (id)delegate;

- (NSWindow *)keyWindow;
- (NSWindow *)mainWindow;
- (NSEvent*) currentEvent;
- (NSEvent *)nextEventMatchingMask:(NSEventMask)mask
                         untilDate:(NSDate *)untilDate
                            inMode:(NSString *)mode
                           dequeue:(BOOL)dequeue;
- (void)sendEvent:(NSEvent *)event;

- (void)updateWindows;

- (void)finishLaunching;
- (BOOL)isActive;
- (void)setPresentationOptions:(NSApplicationPresentationOptions)options;
- (BOOL)isHidden;
- (BOOL)setActivationPolicy:(NSApplicationActivationPolicy)activationPolicy;
- (void)activateIgnoringOtherApps:(BOOL)flag;
@end

@protocol NSApplicationDelegate <NSObject>
@optional
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication;

- (BOOL)applicationShouldHandleReopen:(NSApplication *)application hasVisibleWindows:(BOOL)flag;

- (BOOL)applicationShouldAutomaticallyLocalizeKeyEquivalents:(NSApplication *)application;

- (NSError *)application:(NSApplication *)application willPresentError:(NSError *)error;

- (BOOL)application:(NSApplication *)application willContinueUserActivityWithType:(NSString *)userActivityType;
- (BOOL)application:(NSApplication *)application
    continueUserActivity:(NSUserActivity *)userActivity
      restorationHandler:(void (^)(NSArray<id> *restorableObjects))restorationHandler;
- (void)application:(NSApplication *)application
    didFailToContinueUserActivityWithType:(NSString *)userActivityType
                                    error:(NSError *)error;
- (void)application:(NSApplication *)application didUpdateUserActivity:(NSUserActivity *)userActivity;

- (void)application:(NSApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken;
- (void)application:(NSApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error;
- (void)application:(NSApplication *)application didReceiveRemoteNotification:(NSDictionary<NSString *, id> *)userInfo;
- (void)application:(NSApplication *)application userDidAcceptCloudKitShareWithMetadata:(CKShareMetadata *)metadata;

- (id)application:(NSApplication *)application handlerForIntent:(INIntent *)intent;

- (void)application:(NSApplication *)application openURLs:(NSArray<NSURL *> *)urls;

- (BOOL)application:(NSApplication *)application openFile:(NSString *)filename;
- (BOOL)application:(id)sender openFileWithoutUI:(NSString *)filename;
- (BOOL)application:(NSApplication *)application openTempFile:(NSString *)filename;
- (void)application:(NSApplication *)sender openFiles:(NSArray<NSString *> *)filenames;

- (BOOL)applicationShouldOpenUntitledFile:(NSApplication *)sender;
- (BOOL)applicationOpenUntitledFile:(NSApplication *)sender;

- (BOOL)application:(NSApplication *)application printFile:(NSString *)filename;
- (NSApplicationPrintReply)application:(NSApplication *)application
                            printFiles:(NSArray<NSString *> *)fileNames
                          withSettings:(NSDictionary<NSPrintInfoAttributeKey, id> *)printSettings
                       showPrintPanels:(BOOL)showPrintPanels;

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app;
- (void)application:(NSApplication *)app willEncodeRestorableState:(NSCoder *)coder;
- (void)application:(NSApplication *)app didDecodeRestorableState:(NSCoder *)coder;
- (BOOL)application:(NSApplication *)sender delegateHandlesKey:(NSString *)key;

- (void)applicationDidBecomeActive:(NSNotification *)notification;
- (void)applicationDidChangeOcclusionState:(NSNotification *)notification;
- (void)applicationDidChangeScreenParameters:(NSNotification *)notification;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (void)applicationDidHide:(NSNotification *)notification;
- (void)applicationDidResignActive:(NSNotification *)notification;
- (void)applicationDidUnhide:(NSNotification *)notification;
- (void)applicationDidUpdate:(NSNotification *)notification;
- (void)applicationProtectedDataDidBecomeAvailable:(NSNotification *)notification;
- (void)applicationProtectedDataWillBecomeUnavailable:(NSNotification *)notification;
- (void)applicationWillBecomeActive:(NSNotification *)notification;
- (void)applicationWillFinishLaunching:(NSNotification *)notification;
- (void)applicationWillHide:(NSNotification *)notification;
- (void)applicationWillResignActive:(NSNotification *)notification;
- (void)applicationWillTerminate:(NSNotification *)notification;
- (void)applicationWillUnhide:(NSNotification *)notification;
- (void)applicationWillUpdate:(NSNotification *)notification;
@end

extern NSApplication *NSApp;
