#ifndef SERIALIZER_TYPED_BYTE_ARRAY_BUFFER_HPP
#define SERIALIZER_TYPED_BYTE_ARRAY_BUFFER_HPP

#include "assert.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/exception.hpp"
#include "serializer/type_register.hpp"
#include "byte_array/encoders.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

class TypedByte_ArrayBuffer {
 public:
  typedef byte_array::ByteArray byte_array_type;
  TypedByte_ArrayBuffer() {  detailed_assert( size() == 0 ); }
  TypedByte_ArrayBuffer(byte_array_type s) { data_ = s;  }

  void Allocate(std::size_t const &val) {
    data_.Resize(data_.size() + val);
  }

  void WriteBytes(uint8_t const *arr, std::size_t const &size) {
    for (std::size_t i = 0; i < size; ++i) data_[pos_++] = arr[i];
  }

  void ReadBytes(uint8_t *arr, std::size_t const &size) {
    //    assert(false);
    if( size > bytes_left() ) {
      std::cerr << "Failed to deserialize following string: " << data_ << std::endl;
      std::cerr << byte_array::ToHex( data_ ) << std::endl;
    }
    detailed_assert(size <= bytes_left());
    for (std::size_t i = 0; i < size; ++i) arr[i] = data_[pos_++];
  }

  void SkipBytes(std::size_t const &size) { pos_ += size; }

  
  template <typename T>
  TypedByte_ArrayBuffer &operator<<(T const &val) {
    Serialize(*this, TypeRegister<T>::name);
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  TypedByte_ArrayBuffer &operator>>(T &val) {
    byte_array_type type;
    Deserialize(*this, type);
    if (TypeRegister<T>::name != type) {
      throw SerializableException(error::TYPE_ERROR,
                                  byte_array_type("Type ") + type +
                                      byte_array_type(" differs from ") +
                                      TypeRegister<T>::name);
    }
    Deserialize(*this, val);
    return *this;
  }

  void Seek(std::size_t const &p) { pos_ = p; }
  std::size_t Tell() const { return pos_; }

  std::size_t size() const { return data_.size(); }
  int64_t bytes_left() const { return data_.size() - pos_; }    
  byte_array_type const &data() const { return data_; }

 private:
  byte_array_type data_;
  std::size_t pos_ = 0;
};
};
};

#endif
