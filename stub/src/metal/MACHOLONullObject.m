#import "MACHOLONullObject.h"
#import <Foundation/NSDictionary.h>
#import <Foundation/NSSet.h>
#import <Foundation/NSValue.h>
#include <ctype.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <string.h>
#include <common.h>

// ---------------------------------------------------------------------------
// Discovery log
// ---------------------------------------------------------------------------

static pthread_mutex_t note_lock = PTHREAD_MUTEX_INITIALIZER;
static NSMutableSet *note_seen;

void macholo_metal_note(const char *cls, const char *sel, char kind) {
  pthread_mutex_lock(&note_lock);
  if (note_seen == nil) note_seen = [[NSMutableSet alloc] init];
  NSString *key = [NSString stringWithFormat:@"%c[%s %s]", kind, cls, sel];
  BOOL fresh = ![note_seen containsObject:key];
  if (fresh) [note_seen addObject:key];
  pthread_mutex_unlock(&note_lock);

  if (fresh) fprintf(stderr, "[metal-surface] %c[%s %s]\n", kind, cls, sel);
}

// ---------------------------------------------------------------------------
// Generic IMPs installed for unknown selectors
// ---------------------------------------------------------------------------

// "setFoo:" -> "foo". Anything else is used verbatim minus its colons.
static void normalize_key(const char *sel, char *out, size_t out_len) {
  const char *p = sel;
  if (strncmp(p, "set", 3) == 0 && isupper((unsigned char)p[3])) p += 3;
  else if (strncmp(p, "is", 2) == 0 && isupper((unsigned char)p[2])) p += 2;

  size_t n = 0;
  for (; *p && n + 1 < out_len; ++p) {
    if (*p == ':') continue;
    char c = (n == 0) ? (char)tolower((unsigned char)*p) : *p;
    out[n++] = c;
  }
  out[n] = '\0';
}

static void null_setter(id self, SEL _cmd, uintptr_t value) {
  char key[128];
  normalize_key(sel_getName(_cmd), key, sizeof key);
  [self macholoSetValue:value forKey:key];
}

static uintptr_t null_getter(id self, SEL _cmd) {
  char key[128];
  normalize_key(sel_getName(_cmd), key, sizeof key);
  return [self macholoValueForKey:key];
}

static uintptr_t null_generic(id self, SEL _cmd, ...) {
  (void)self;
  (void)_cmd;
  return 0;
}

static BOOL add_fallback(Class target, const char *owner, SEL sel, char kind) {
  const char *name = sel_getName(sel);
  macholo_metal_note(owner, name, kind);

  size_t colons = 0;
  for (const char *p = name; *p; ++p)
    if (*p == ':') colons++;

  if (colons == 0) return class_addMethod(target, sel, (IMP)null_getter, "Q@:");

  if (colons == 1 && strncmp(name, "set", 3) == 0 && isupper((unsigned char)name[3]) &&
      name[strlen(name) - 1] == ':')
    return class_addMethod(target, sel, (IMP)null_setter, "v@:Q");

  // "@@:" plus one word-sized argument per colon. The encoding only matters for
  // NSInvocation-based reflection, never for the direct dispatch the game does.
  char types[160];
  size_t n = 0;
  types[n++] = '@';
  types[n++] = '@';
  types[n++] = ':';
  for (size_t i = 0; i < colons && n + 1 < sizeof types; ++i) types[n++] = '@';
  types[n] = '\0';
  return class_addMethod(target, sel, (IMP)null_generic, types);
}

// ---------------------------------------------------------------------------

@implementation MACHOLONullObject {
  NSMutableDictionary *_bag;
}
@synthesize label = _label;

- (uintptr_t)macholoValueForKey:(const char *)key {
  NSNumber *v = [_bag objectForKey:[NSString stringWithUTF8String:key]];
  return v ? (uintptr_t)[v unsignedLongLongValue] : 0;
}

- (void)macholoSetValue:(uintptr_t)value forKey:(const char *)key {
  if (_bag == nil) _bag = [[NSMutableDictionary alloc] init];
  [_bag setObject:[NSNumber numberWithUnsignedLongLong:value]
           forKey:[NSString stringWithUTF8String:key]];
}

+ (BOOL)resolveInstanceMethod:(SEL)sel {
  return add_fallback(self, class_getName(self), sel, '-');
}

+ (BOOL)resolveClassMethod:(SEL)sel {
  return add_fallback(object_getClass(self), class_getName(self), sel, '+');
}

- (void)dealloc {
  [_bag release];
  [_label release];
  [super dealloc];
}
@end
