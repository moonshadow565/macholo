#include "tlv.hpp"

#include <pthread.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "common.hpp"
#include "loader.hpp"
#include "macho.hpp"

struct tlv_descriptor_t {
  void* (*thunk)(tlv_descriptor_t*);
  pthread_t key;
  uintptr_t offset;
};

static void* tlv_allocate_for_key(pthread_key_t key) {
  auto const image = loader::Image::main;
  void* data = malloc(image->object.tlv_size);
  if (!data) return nullptr;
  memset(data, 0, image->object.tlv_size);
  memcpy(data, (void*)(image->object.tlv_start + image->slide), image->object.tlv_size);

  // NOTE: publish the block before running the initializers -- an initializer may itself
  // touch a thread-local, which would otherwise recurse back in here and hand out a
  // second, competing block for the same thread.
  pthread_setspecific(key, data);

  for (auto const& seg : image->object.segments) {
    for (auto const& sect : seg.sections) {
      if (sect.type != macho::Section::TYPE_THREAD_LOCAL_INIT_FUNCTION_POINTERS) {
        continue;
      }
      for (auto i = sect.addr; i < sect.addr + sect.size; i += sizeof(uintptr_t)) {
        auto const func = (void (**)())(i + image->slide);
        (*func)();
      }
    }
  }

  return data;
}

static void tlv_destroy(void*) {
  // TODO
  return;
}

[[clang::preserve_all]] static void* tlv_thunk(tlv_descriptor_t* desc) {
  void* result = pthread_getspecific(desc->key);
  if (!result) {
    result = tlv_allocate_for_key(desc->key);
    assert_that(result != nullptr);
  }
  return (void*)((char*)result + desc->offset);
}

void tlv::fixup_image(loader::Image* image) {
  pthread_key_t key;
  int key_error = pthread_key_create(&key, &tlv_destroy);
  assert_that(key_error == 0);

  for (auto const& seg : image->object.segments) {
    for (auto const& sect : seg.sections) {
      if (sect.type != macho::Section::TYPE_THREAD_LOCAL_VARIABLES) {
        continue;
      }
      for (auto i = sect.addr; i < sect.addr + sect.size; i += sizeof(tlv_descriptor_t)) {
        auto const desc = (tlv_descriptor_t*)(i + image->slide);
        desc->thunk = (decltype(tlv_descriptor_t::thunk))&tlv_thunk;
        desc->key = key;
      }
    }
  }
}
