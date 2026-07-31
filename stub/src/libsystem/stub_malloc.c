#include <malloc.h>

#include <common.h>

size_t $darwin$malloc_size(void const *ptr) {
  return ptr ? malloc_usable_size((void *)ptr) : 0;
}
