/* Build-time sanity check for the GNUstep sysroot (see Containerfile step 6).
 * Exercises the three libraries the macholo stub links: the Objective-C
 * runtime (libobjc), Foundation (libgnustep-base) and CoreFoundation
 * (libgnustep-corebase), using the same compile flags as the consumer. */
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <stdio.h>

int main(void)
{
  @autoreleasepool
    {
      NSString *s = @"hello from gnustep";
      if (![s isKindOfClass: [NSString class]])
        {
          fprintf(stderr, "smoke: constant string is not an NSString\n");
          return 1;
        }
      printf("objc:  %s ok, isa=%s\n", [s UTF8String], class_getName([s class]));

      /* blocks + libdispatch-backed machinery */
      NSArray *a = @[@"a", @"b", @"c"];
      __block NSUInteger n = 0;
      [a enumerateObjectsUsingBlock: ^(id obj, NSUInteger idx, BOOL *stop) { n++; }];
      if (n != 3)
        {
          fprintf(stderr, "smoke: block enumeration gave %lu\n", (unsigned long)n);
          return 1;
        }
      printf("base:  blocks ok, NSArray count=%lu\n", (unsigned long)[a count]);

      CFStringRef cf = CFStringCreateWithBytes(NULL, (const UInt8 *)"cf", 2,
                                               kCFStringEncodingUTF8, false);
      CFArrayRef  ca = CFArrayCreate(NULL, NULL, 0, NULL);
      if (cf == NULL || ca == NULL || CFStringGetLength(cf) != 2)
        {
          fprintf(stderr, "smoke: corebase failed\n");
          return 1;
        }
      printf("cf:    CFString len=%ld, CFArray count=%ld\n",
             (long)CFStringGetLength(cf), (long)CFArrayGetCount(ca));
      CFRelease(cf);
      CFRelease(ca);

      /* exception handling relies on the libobjc2 / libc++abi interop */
      @try
        {
          [NSException raise: @"SmokeTest" format: @"expected"];
        }
      @catch (NSException *e)
        {
          printf("eh:    caught %s\n", [[e name] UTF8String]);
        }
    }
  printf("smoke: OK\n");
  return 0;
}
