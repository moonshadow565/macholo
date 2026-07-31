#include <Foundation/NSValue.h>

// Apple's -[NSException callStackReturnAddresses] hands back NSNumbers; GNUstep hands back
// GSPointerValue NSValues (objCType "^v"). Code that catches an exception and logs its
// stack calls -unsignedIntegerValue on each element -- which a bare NSValue does not
// implement -- raising a *second*, uncaught exception that aborts the process and masks
// the original error. WC3 does exactly this from its @catch handler, so any single
// unrecognized selector anywhere becomes a hard crash. Give NSValue the numeric accessors
// so a pointer-valued NSValue yields its address as an integer, which is what Apple's
// NSNumber-wrapped addresses would have produced. NSNumber defines its own versions, so
// genuine numbers are unaffected (subclass methods win over this category on the base).
@interface NSValue (MacholoNumeric)
@end
@implementation NSValue (MacholoNumeric)
- (NSUInteger)unsignedIntegerValue {
  NSUInteger v = 0;
  [self getValue:&v];
  return v;
}
- (NSInteger)integerValue {
  NSInteger v = 0;
  [self getValue:&v];
  return v;
}
@end
