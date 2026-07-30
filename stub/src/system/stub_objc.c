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
