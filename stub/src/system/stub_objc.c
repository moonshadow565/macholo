#include <common.h>

void *_objc_empty_cache = NULL;

void *objc_empty_vtable = NULL;

// Modern (10.15+) objc entry points. libobjc2 v2.3 already provides objc_alloc and
// objc_alloc_init; the objc_opt_* family is Apple-only, so implement it here.
typedef struct objc_object *id;
typedef struct objc_class *Class;
typedef struct objc_selector *SEL;
typedef signed char BOOL;

extern SEL sel_registerName(const char *name);
extern id objc_msgSend(id self, SEL op, ...);
extern Class object_getClass(id obj);
extern Class class_getSuperclass(Class cls);
extern BOOL class_isMetaClass(Class cls);

typedef struct objc_ivar *Ivar;
extern Ivar class_getInstanceVariable(Class cls, const char *name);
extern id object_getIvar(id obj, Ivar ivar);
extern void object_setIvar(id obj, Ivar ivar, id value);

// Apple vs GNUstep object_{get,set}InstanceVariable ABI mismatch.
//
// GNUstep libobjc2's object_getInstanceVariable writes the *address* of the ivar into
// *outValue (ivar.c: `*outValue = (char*)obj + ivar_getOffset(ivar)`), and its setter
// dereferences the `value` argument (`object_setIvar(obj, ivar, *(id*)value)`). Apple's
// runtime -- which this Mach-O game was compiled against -- passes/returns the ivar's
// *value* directly (object_getIvar / object_setIvar). Under the GNUstep semantics a nil
// object ivar comes back as a non-nil address, so the game's `if (ivar) [ivar msg]` guard
// passes and it messages garbage. WC3 hits this on MacApplication's nil `_mainCoreWindow`
// during startup: [coreWindow isVisible] on a not-yet-created window -> crash. The
// $darwin$ prefix makes lib::resolve prefer these over libobjc2's.
Ivar $darwin$object_getInstanceVariable(id obj, const char *name, void **outValue) {
  if (!obj) {
    if (outValue) *outValue = (void *)0;
    return (Ivar)0;
  }
  Ivar const ivar = class_getInstanceVariable(object_getClass(obj), name);
  if (outValue) *outValue = ivar ? (void *)object_getIvar(obj, ivar) : (void *)0;
  return ivar;
}

Ivar $darwin$object_setInstanceVariable(id obj, const char *name, void *value) {
  if (!obj) return (Ivar)0;
  Ivar const ivar = class_getInstanceVariable(object_getClass(obj), name);
  if (ivar) object_setIvar(obj, ivar, (id)value);
  return ivar;
}

// [obj class], nil safe. For a class object object_getClass() returns the metaclass,
// but +class returns the class itself, hence the metaclass check.
Class objc_opt_class(id obj) {
  if (!obj) return (Class)0;
  Class const cls = object_getClass(obj);
  if (class_isMetaClass(cls)) return (Class)obj;
  return cls;
}

// [obj isKindOfClass:cls], nil safe.
BOOL objc_opt_isKindOfClass(id obj, Class cls) {
  if (!obj || !cls) return 0;
  for (Class t = objc_opt_class(obj); t; t = class_getSuperclass(t))
    if (t == cls) return 1;
  return 0;
}

// [cls new]
id objc_opt_new(Class cls) {
  if (!cls) return (id)0;
  static SEL sel = (SEL)0;
  if (!sel) sel = sel_registerName("new");
  return objc_msgSend((id)cls, sel);
}

// [obj self]
id objc_opt_self(id obj) {
  if (!obj) return (id)0;
  static SEL sel = (SEL)0;
  if (!sel) sel = sel_registerName("self");
  return objc_msgSend(obj, sel);
}

void *objc_msgSend_noarg();
void *objc_msgSendSuper();
void *objc_msgSendSuper2();
void *objc_msgSendSuper2_stret();
void *objc_msgSend_fixup();
void *objc_msgSendSuper2_fixup();
void *objc_msgSendSuper2_stret_fixup();
void *objc_msgSend_fpret_fixup();
void *objc_msgSend_fp2ret_fixup();
void *objc_msgSend_stret_fixup();
