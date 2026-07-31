// mini-CF: CFArray == NSArray, CFMutableArrayRef == NSMutableArray *. See minicf.h.
#include <Foundation/NSArray.h>
#include <common.h>

#include "minicf.h"

// The callback set every caller in the games passes. NS containers already retain and
// release their elements and compare them with -isEqual:, so mini-CF ignores the
// callbacks argument entirely -- but the struct still has to exist, because the game
// imports the symbol and takes its address. Filling it in honestly (rather than
// zeroing it) keeps it usable if anything ever calls through it.
static const void *cf_type_retain(CFAllocatorRef allocator, const void *value) {
  return CFRetain(value);
}
static void cf_type_release(CFAllocatorRef allocator, const void *value) {
  CFRelease(value);
}
static Boolean cf_type_equal(const void *value1, const void *value2) {
  if (value1 == value2) return true;
  if (value1 == NULL || value2 == NULL) return false;
  return [(id)value1 isEqual:(id)value2] ? true : false;
}

const CFArrayCallBacks kCFTypeArrayCallBacks = {
    .version = 0,
    .retain = cf_type_retain,
    .release = cf_type_release,
    .copyDescription = NULL,  // only used by CFCopyDescription, which nothing imports
    .equal = cf_type_equal,
};

CFArrayRef CFArrayCreate(CFAllocatorRef allocator, const void **values, CFIndex numValues,
                         const CFArrayCallBacks *callBacks) {
  if (numValues < 0) return NULL;
  if (values == NULL && numValues != 0) return NULL;
  // Create rule: +1.
  return (CFArrayRef)[[NSArray alloc] initWithObjects:(id const *)values count:(NSUInteger)numValues];
}

CFMutableArrayRef CFArrayCreateMutable(CFAllocatorRef allocator, CFIndex capacity,
                                       const CFArrayCallBacks *callBacks) {
  // CF's capacity 0 means "unlimited"; NSMutableArray's is only a hint either way.
  return (CFMutableArrayRef)[[NSMutableArray alloc] initWithCapacity:capacity > 0 ? (NSUInteger)capacity : 0];
}

void CFArrayAppendValue(CFMutableArrayRef theArray, const void *value) {
  if (theArray == NULL || value == NULL) return;
  [(NSMutableArray *)theArray addObject:(id)value];
}

CFIndex CFArrayGetCount(CFArrayRef theArray) {
  if (theArray == NULL) return 0;
  return (CFIndex)[(NSArray *)theArray count];
}

const void *CFArrayGetValueAtIndex(CFArrayRef theArray, CFIndex idx) {
  // Get rule: +0. Out of range is undefined in CF (it reads past the end); NSArray would
  // raise, so bounds-check and answer NULL instead.
  if (theArray == NULL || idx < 0) return NULL;
  NSArray *array = (NSArray *)theArray;
  if ((NSUInteger)idx >= [array count]) return NULL;
  return (const void *)[array objectAtIndex:(NSUInteger)idx];
}
