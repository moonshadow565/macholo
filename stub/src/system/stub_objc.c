#include <common.h>

void *_objc_empty_cache = NULL;

void *objc_empty_vtable = NULL;

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
