#ifndef SERIALIZER_BYTE_ARRAY_BUFFER_HPP
#define SERIALIZER_BYTE_ARRAY_BUFFER_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "assert.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

class ByteArrayBuffer {
 public:
  ByteArrayBuffer() {}
  ByteArrayBuffer(byte_array::ByteArray s) { data_ = s; }

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
  ByteArrayBuffer &operator<<(T const &val) {
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  ByteArrayBuffer &operator>>(T &val) {
    Deserialize(*this, val);
    return *this;
  }

  template <typename T>
  ByteArrayBuffer &Pack(T const &val) {
    return this->operator<<(val);
  }

  template <typename T>
  ByteArrayBuffer &Unpack(T &val){
    return this->operator>>(val);
  }

  // FIXME: Incorrect naming
  void Seek(std::size_t const &p) { pos_ = p; }
  std::size_t Tell() const { return pos_; }

  std::size_t size() const { return data_.size(); }
  int64_t bytes_left() const { return data_.size() - pos_; }  
  byte_array::ByteArray const &data() const { return data_; }

 private:
  byte_array::ByteArray data_;

  std::size_t pos_ = 0;
};
};
};

#endif
