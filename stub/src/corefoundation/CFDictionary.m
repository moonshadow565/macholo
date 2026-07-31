// mini-CF: CFDictionary == NSDictionary. See minicf.h.
#include <Foundation/NSArray.h>
#include <Foundation/NSDictionary.h>
#include <Foundation/NSEnumerator.h>
#include <common.h>

#include "minicf.h"

CFDictionaryRef CFDictionaryCreate(CFAllocatorRef allocator, const void **keys, const void **values,
                                   CFIndex numValues, const CFDictionaryKeyCallBacks *keyCallBacks,
                                   const CFDictionaryValueCallBacks *valueCallBacks) {
  if (numValues < 0) return NULL;
  if (numValues != 0 && (keys == NULL || values == NULL)) return NULL;
  // Callbacks ignored, as everywhere in mini-CF. One real divergence from
  // kCFTypeDictionaryKeyCallBacks, which *retains* keys: NSDictionary *copies* them, so
  // a key that doesn't conform to NSCopying would raise here where CF would not. Every
  // key the games use is a string, for which -copy is -retain.
  return (CFDictionaryRef)[[NSDictionary alloc] initWithObjects:(id const *)values
                                                        forKeys:(id const *)keys
                                                          count:(NSUInteger)numValues];
}

const void *CFDictionaryGetValue(CFDictionaryRef theDict, const void *key) {
  // Get rule: +0.
  if (theDict == NULL || key == NULL) return NULL;
  return (const void *)[(NSDictionary *)theDict objectForKey:(id)key];
}

Boolean CFDictionaryContainsValue(CFDictionaryRef theDict, const void *value) {
  if (theDict == NULL || value == NULL) return false;
  // kCFTypeDictionaryValueCallBacks compares with CFEqual, i.e. -isEqual:, which is
  // what -containsObject: uses.
  return [[(NSDictionary *)theDict allValues] containsObject:(id)value] ? true : false;
}

void CFDictionaryApplyFunction(CFDictionaryRef theDict, CFDictionaryApplierFunction applier, void *context) {
  if (theDict == NULL || applier == NULL) return;
  NSDictionary *dict = (NSDictionary *)theDict;
  NSEnumerator *keys = [dict keyEnumerator];
  for (id key = [keys nextObject]; key != nil; key = [keys nextObject])
    applier((const void *)key, (const void *)[dict objectForKey:key], context);
}
