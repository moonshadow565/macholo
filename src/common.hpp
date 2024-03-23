#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string_view>
#include <utility>

#define debug_break() asm("nop ; int3 ; nop ")

//#define log_objc(fmt, ...) fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__)
#define log_objc(fmt, ...)

#define warn_unresolved(fmt, ...) fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__)
//#define warn_unresolved(fmt, ...)

#define panic(fmt, ...)                                                          \
  do {                                                                           \
    fprintf(stderr, "%s:%d" fmt, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
    asm("int $3");                                                               \
    abort();                                                                     \
  } while (0)

#define assert_that(...)                           \
  do {                                             \
    if (!(__VA_ARGS__)) panic("%s", #__VA_ARGS__); \
  } while (0)

struct unique_fd {
  constexpr unique_fd() : fd_(-1) {}

  constexpr unique_fd(int fd) : fd_(fd) {}

  constexpr unique_fd(unique_fd const&) = delete;

  constexpr unique_fd(unique_fd&& other) : fd_(std::exchange(fd_, -1)) {}

  constexpr void operator=(unique_fd& other) = delete;

  constexpr void operator=(unique_fd&& other) { std::swap(fd_, other.fd_); }

  ~unique_fd() noexcept { close(); }

  explicit constexpr operator bool() const { return fd_ != -1; }

  constexpr bool operator!() const { return fd_ == -1; }

  constexpr int get() const { return fd_; }

  void close() noexcept {
    if (auto fd = std::exchange(fd_, -1); fd_ != -1) ::close(fd);
  }

  size_t size() const {
    struct stat statbuf;
    if (::fstat(fd_, &statbuf) != 0) return 0;
    return statbuf.st_size;
  }

private:
  int fd_;
};

template<typename T>
struct slice {
  T* data_;
  size_t size_;

  T* data() const { return data_;}
  size_t size() const { return size_; }
  bool empty() const  { return !data_ || size_; }
  T& operator[](size_t i) const { return data_[i]; }
  T* begin() const { return data_; }
  T* end() const { return data_ + size_; }
};
