#include "interprocess_connect.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdexcept>

namespace ulanova
{
  Mapping::Mapping():
    ptr_(nullptr),
    size_(0),
    fd_(-1),
    is_owner_(false)
  {}

  Mapping::Mapping(Mapping&& other) noexcept:
    ptr_(other.ptr_),
    size_(other.size_),
    fd_(other.fd_),
    is_owner_(other.is_owner_)
  {
    other.ptr_ = nullptr;
    other.size_ = 0;
    other.fd_ = -1;
    other.is_owner_ = false;
  }

  Mapping& Mapping::operator=(Mapping&& other) noexcept
  {
    if (this != &other)
    {
      close();
      ptr_ = other.ptr_;
      size_ = other.size_;
      fd_ = other.fd_;
      is_owner_ = other.is_owner_;

      other.ptr_ = nullptr;
      other.size_ = 0;
      other.fd_ = -1;
      other.is_owner_ = false;
    }
    return *this;
  }

  Mapping::~Mapping()
  {
    close();
  }

  void Mapping::create(const char* name, size_t size)
  {
    close();
    size_ = size;
    is_owner_ = true;

    fd_ = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_ == -1)
    {
      throw std::runtime_error("Failed to create shared memory object");
    }

    if (ftruncate(fd_, static_cast< off_t >(size)) == -1)
    {
      ::close(fd_);
      shm_unlink(name);
      throw std::runtime_error("Failed to set size of shared memory");
    }

    ptr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (ptr_ == MAP_FAILED)
    {
      ::close(fd_);
      shm_unlink(name);
      throw std::runtime_error("Failed to map shared memory");
    }
  }

  void Mapping::open(const char* name, size_t size)
  {
    close();
    size_ = size;
    is_owner_ = false;

    fd_ = shm_open(name, O_RDONLY, 0);
    if (fd_ == -1)
    {
      throw std::runtime_error("Failed to open shared memory object");
    }

    ptr_ = mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (ptr_ == MAP_FAILED)
    {
      ::close(fd_);
      throw std::runtime_error("Failed to map shared memory");
    }
  }

  void Mapping::close()
  {
    if (ptr_ && ptr_ != MAP_FAILED)
    {
      munmap(ptr_, size_);
    }
    if (fd_ != -1)
    {
      ::close(fd_);
    }
    ptr_ = nullptr;
    fd_ = -1;
    size_ = 0;
    is_owner_ = false;
  }

  void* Mapping::getPtr() const noexcept
  {
    return ptr_;
  }

  void sendAll(int fd, const void* buf, size_t size)
  {
    size_t total_written = 0;
    const char* const ptr = static_cast< const char* >(buf);

    while (total_written < size)
    {
      const ssize_t written = write(fd, ptr + total_written, size - total_written);
      if (written <= 0)
      {
        throw std::runtime_error("Failed to write to pipe");
      }
      total_written += static_cast< size_t >(written);
    }
  }

  void recvAll(int fd, void* buf, size_t size)
  {
    size_t total_read = 0;
    char* const ptr = static_cast< char* >(buf);

    while (total_read < size)
    {
      const ssize_t bytes_read = read(fd, ptr + total_read, size - total_read);
      if (bytes_read <= 0)
      {
        throw std::runtime_error("Failed to read from pipe");
      }
      total_read += static_cast< size_t >(bytes_read);
    }
  }
}
