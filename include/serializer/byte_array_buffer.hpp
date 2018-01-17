#ifndef SERIALIZER_BYTE_ARRAY_BUFFER_HPP
#define SERIALIZER_BYTE_ARRAY_BUFFER_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "assert.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

class Byte_ArrayBuffer {
 public:
  Byte_ArrayBuffer() {}
  Byte_ArrayBuffer(byte_array::ReferencedByteArray s) { data_ = s; }

  void Allocate(std::size_t const &val) {
    data_.Resize(data_.size() + val);
  }

  void WriteBytes(uint8_t const *arr, std::size_t const &size) {
    for (std::size_t i = 0; i < size; ++i) data_[pos_++] = arr[i];
  }

  void ReadBytes(uint8_t *arr, std::size_t const &size) {
    for (std::size_t i = 0; i < size; ++i) arr[i] = data_[pos_++];
  }

  void SkipBytes(std::size_t const &size) { pos_ += size; }

  template <typename T>
  Byte_ArrayBuffer &operator<<(T const &val) {
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  Byte_ArrayBuffer &operator>>(T &val) {
    Deserialize(*this, val);
    return *this;
  }

  // FIXME: Incorrect naming
  void Seek(std::size_t const &p) { pos_ = p; }
  std::size_t Tell() const { return pos_; }

  std::size_t size() const { return data_.size(); }
  int64_t bytes_left() const { return data_.size() - pos_; }  
  byte_array::ReferencedByteArray const &data() const { return data_; }

 private:
  byte_array::ReferencedByteArray data_;

  std::size_t pos_ = 0;
};
};
};

#endif
