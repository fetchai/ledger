#ifndef SERIALIZER_COUNTER_HPP
#define SERIALIZER_COUNTER_HPP
#include "core/serializers/stl_types.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/type_register.hpp"
#include <type_traits>

namespace fetch {
namespace serializers {

class TypedByte_ArrayBuffer;

template <typename S>
class SizeCounter
{
public:
  void Allocate(std::size_t const &val) { size_ += val; }

  void Reserve(std::size_t const &val) {}

  void WriteBytes(uint8_t const *arr, std::size_t const &size) {}
  void ReadBytes(uint8_t const *arr, std::size_t const &size) {}

  void SkipBytes(std::size_t const &size) {}

  template <typename T>
  SizeCounter &operator<<(T const &val)
  {
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  SizeCounter &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  void        Seek(std::size_t const &p) {}
  std::size_t Tell() const { return 0; }
  int64_t     bytes_left() const { return 0; }
  std::size_t size() const { return size_; }

private:
  std::size_t size_ = 0;
};

template <>
class SizeCounter<TypedByte_ArrayBuffer>
{
public:
  void Allocate(std::size_t const &val) { size_ += val; }

  void Reserve(std::size_t const &val) {}

  void WriteBytes(uint8_t const *arr, std::size_t const &size) {}
  void ReadBytes(uint8_t const *arr, std::size_t const &size) {}

  void SkipBytes(std::size_t const &size) {}

  template <typename T>
  SizeCounter &operator<<(T const &val)
  {
    Serialize(*this, TypeRegister<void>::value_type(TypeRegister<T>::value));
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  SizeCounter &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  void        Seek(std::size_t const &p) {}
  std::size_t Tell() const { return 0; }

  int64_t bytes_left() const { return 0; }

  std::size_t size() const { return size_; }

private:
  std::size_t size_ = 0;
};
}  // namespace serializers
}  // namespace fetch

#endif
