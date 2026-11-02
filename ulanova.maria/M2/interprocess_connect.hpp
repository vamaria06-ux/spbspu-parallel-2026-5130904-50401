#ifndef INTERPROCESS_CONNECT_HPP
#define INTERPROCESS_CONNECT_HPP

#include <cstddef>
#include "area_calc.hpp"

namespace ulanova
{
  enum class cmd_t : char
  {
    CALC,
    EXIT
  };

  enum class status_t : char
  {
    SUCCESS,
    ERROR
  };

  struct request_t
  {
    cmd_t command;
    size_t shapes_count;
    rect_t rect;
    size_t thrds;
    size_t tests;
    size_t seed;
  };
  struct response_t
  {
    status_t status;
    size_t hits;
  };

  class Mapping
  {
  public:
    Mapping();
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    Mapping(Mapping&& other) noexcept;
    Mapping& operator=(Mapping&& other) noexcept;
    ~Mapping();

    void create(const char* name, size_t size);
    void open(const char* name, size_t size);
    void close();

    void* getPtr() const noexcept;

  private:
    void* ptr_;
    size_t size_;
    int fd_;
    bool is_owner_;
  };

  void sendAll(int fd, const void* buf, size_t size);
  void recvAll(int fd, void* buf, size_t size);
}

#endif
