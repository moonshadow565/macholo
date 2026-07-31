// Carbon / HIToolbox surface WC3 still links against: the Text Input Sources API
// (keyboard layout discovery), a few legacy event-state accessors, the symbolic
// hot-key list, and the FSRef file APIs.
//
// SDL already delivers translated key events (see NSEvent.m), so the game's Carbon
// keyboard path is a fallback it never needs to succeed on -- reporting "no input
// sources" keeps it on the Cocoa path. The hot-key and secure-event-input calls are
// window-server state that does not exist here.
#include <stdint.h>
#include <string.h>

#include <common.h>

typedef int32_t OSStatus;
typedef uint32_t OptionBits;
typedef uint16_t UniChar;
typedef uint32_t UniCharCount;
typedef const void *CFTypeRef;
typedef const void *CFStringRef;
typedef const void *CFArrayRef;
typedef const void *CFDictionaryRef;
typedef const void *CFURLRef;
typedef const void *CFAllocatorRef;
typedef void *TISInputSourceRef;

enum { noErr = 0, paramErr = -50, fnfErr = -43, eventNotHandledErr = -9874 };

// ---------------------------------------------------------------------------
// Text Input Sources
// ---------------------------------------------------------------------------

// DATA: property/notification keys. The game only ever passes these straight back to
// TISGetInputSourceProperty, which has no source to look them up on.
CFStringRef kTISPropertyInputSourceCategory = (CFStringRef) "TSMInputSourceCategory";
CFStringRef kTISPropertyInputSourceType = (CFStringRef) "TSMInputSourceType";
CFStringRef kTISPropertyInputSourceID = (CFStringRef) "TSMInputSourceID";
CFStringRef kTISPropertyInputModeID = (CFStringRef) "TSMInputModeID";
CFStringRef kTISPropertyLocalizedName = (CFStringRef) "TSMInputSourceLocalizedName";
CFStringRef kTISPropertyInputSourceLanguages = (CFStringRef) "TSMInputSourceLanguages";
CFStringRef kTISPropertyUnicodeKeyLayoutData = (CFStringRef) "TSMInputSourceKeyLayoutData";
CFStringRef kTISPropertyIconImageURL = (CFStringRef) "TSMInputSourceIconImageURL";
CFStringRef kTISPropertyIconRef = (CFStringRef) "TSMInputSourceIconRef";
CFStringRef kTISPropertyInputSourceIsASCIICapable = (CFStringRef) "TSMInputSourceIsASCIICapable";
CFStringRef kTISPropertyInputSourceIsEnableCapable = (CFStringRef) "TSMInputSourceIsEnableCapable";
CFStringRef kTISPropertyInputSourceIsSelectCapable = (CFStringRef) "TSMInputSourceIsSelectCapable";
CFStringRef kTISPropertyInputSourceIsEnabled = (CFStringRef) "TSMInputSourceIsEnabled";
CFStringRef kTISPropertyInputSourceIsSelected = (CFStringRef) "TSMInputSourceIsSelected";
CFStringRef kTISCategoryKeyboardInputSource = (CFStringRef) "TISCategoryKeyboardInputSource";
CFStringRef kTISNotifySelectedKeyboardInputSourceChanged = (CFStringRef) "AppleSelectedInputSourcesChangedNotification";
CFStringRef kTISNotifyEnabledKeyboardInputSourcesChanged = (CFStringRef) "AppleEnabledInputSourcesChangedNotification";

TISInputSourceRef TISCopyCurrentKeyboardInputSource(void) {
  return NULL;
}

TISInputSourceRef TISCopyCurrentKeyboardLayoutInputSource(void) {
  return NULL;
}

TISInputSourceRef TISCopyCurrentASCIICapableKeyboardLayoutInputSource(void) {
  return NULL;
}

CFArrayRef TISCreateInputSourceList(CFDictionaryRef properties, unsigned char includeAllInstalled) {
  return NULL;
}

void *TISGetInputSourceProperty(TISInputSourceRef inputSource, CFStringRef propertyKey) {
  return NULL;
}

OSStatus TISSelectInputSource(TISInputSourceRef inputSource) {
  return paramErr;
}

// Translate a virtual key through a 'uchr' layout. There is no layout to translate
// with (TISGetInputSourceProperty above hands out none), so report "produced nothing"
// -- callers then fall back to the character already carried by the Cocoa event.
OSStatus UCKeyTranslate(const void *keyLayoutPtr, uint16_t virtualKeyCode, uint16_t keyAction, uint32_t modifierKeyState,
                        uint32_t keyboardType, OptionBits keyTranslateOptions, uint32_t *deadKeyState,
                        UniCharCount maxStringLength, UniCharCount *actualStringLength, UniChar unicodeString[]) {
  if (actualStringLength) *actualStringLength = 0;
  if (deadKeyState) *deadKeyState = 0;
  return noErr;
}

// ---------------------------------------------------------------------------
// Legacy keyboard/event state
// ---------------------------------------------------------------------------

uint32_t KBGetLayoutType(int16_t iKeyboardType) {
  // kKeyboardISO -- the layout ID matters only for key-cap drawing, which we don't do.
  return 0;
}

uint8_t LMGetKbdType(void) {
  return 0;
}

void GetKeys(uint32_t theKeys[4]) {
  // Physical key bitmap. SDL owns the keyboard here; report nothing held.
  if (theKeys) memset(theKeys, 0, 4 * sizeof(uint32_t));
}

uint32_t GetCurrentEventButtonState(void) {
  return 0;
}

uint32_t GetCurrentEventKeyModifiers(void) {
  return 0;
}

double GetCurrentEventTime(void) {
  extern uint64_t mach_absolute_time(void);
  return (double)mach_absolute_time() / 1e9;
}

// ---------------------------------------------------------------------------
// Secure event input / symbolic hot keys -- window-server state
// ---------------------------------------------------------------------------

OSStatus EnableSecureEventInput(void) {
  return noErr;
}

OSStatus DisableSecureEventInput(void) {
  return noErr;
}

OSStatus CopySymbolicHotKeys(CFArrayRef *outHotKeys) {
  // The game reads this to warn about hot keys that would steal its input. No hot keys
  // exist, so hand back nothing rather than an error it would have to interpret.
  if (outHotKeys) *outHotKeys = NULL;
  return noErr;
}

OSStatus GetSymbolicHotKeyMode(uint32_t *outMode) {
  if (outMode) *outMode = 0;
  return noErr;
}

OSStatus PushSymbolicHotKeyMode(uint32_t inMode) {
  return noErr;
}

OSStatus PopSymbolicHotKeyMode(void) {
  return noErr;
}

unsigned char AXIsProcessTrusted(void) {
  // Accessibility permission -- the game only uses it to decide whether to offer
  // global hot keys. "Not trusted" is both true and the quiet answer.
  return 0;
}

// ---------------------------------------------------------------------------
// Process Manager / FSRef
// ---------------------------------------------------------------------------

OSStatus GetProcessForPID(int pid, void *psn) {
  if (psn) memset(psn, 0, 8);
  return noErr;
}

OSStatus GetProcessBundleLocation(const void *psn, void *outLocation) {
  if (outLocation) memset(outLocation, 0, 80);
  return fnfErr;
}

OSStatus FSFindFolder(int16_t vRefNum, uint32_t folderType, unsigned char createFolder, void *foundRef) {
  // Every caller has an NSSearchPathForDirectoriesInDomains fallback for this failing.
  if (foundRef) memset(foundRef, 0, 80);
  return fnfErr;
}

OSStatus FSRefMakePath(const void *ref, uint8_t *path, uint32_t maxPathSize) {
  if (path && maxPathSize) path[0] = 0;
  return fnfErr;
}

CFURLRef CFURLCreateFromFSRef(CFAllocatorRef allocator, const void *fsRef) {
  return NULL;
}
