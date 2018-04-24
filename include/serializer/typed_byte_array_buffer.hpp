#ifndef SERIALIZER_TYPED_BYTE_ARRAY_BUFFER_HPP
#define SERIALIZER_TYPED_BYTE_ARRAY_BUFFER_HPP

#include "assert.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/exception.hpp"
#include "serializer/type_register.hpp"
#include "byte_array/encoders.hpp"
#include "logger.hpp"
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

  void Reserve(std::size_t const &val) {
    data_.Reserve(data_.size() + val);
  }

  
  void WriteBytes(uint8_t const *arr, std::size_t const &size) {
    for (std::size_t i = 0; i < size; ++i) data_[pos_++] = arr[i];
  }

  void ReadBytes(uint8_t *arr, std::size_t const &size) {
    if( size > bytes_left() ) {
      throw std::runtime_error("Not enough bytes");      
    }
    
    for (std::size_t i = 0; i < size; ++i) arr[i] = data_[pos_++];
  }

  void ReadByteArray(byte_array::BasicByteArray &b, std::size_t const &size) {
    if( size > bytes_left() ) {
      throw std::runtime_error("Not enough bytes II");
    }
    
    b = data_.SubArray(pos_, size);
    pos_ += size;
  }

  
  void SkipBytes(std::size_t const &size) { pos_ += size; }

  
  template <typename T>
  TypedByte_ArrayBuffer &operator<<(T const &val) {
    Serialize(*this, TypeRegister<void>::value_type( TypeRegister<T>::value ) );
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  TypedByte_ArrayBuffer &operator>>(T &val) {
    TypeRegister<void>::value_type type;
    Deserialize(*this, type);
    if (TypeRegister<T>::value != type) {
      fetch::logger.Debug("Serializer at position ", pos_, " out of ", data_.size());  
      fetch::logger.Error(byte_array_type("Type '") + TypeRegister<T>::name() +
        byte_array_type("' differs from type '") + ErrorCodeToMessage(type) +"'");

    
      throw SerializableException(error::TYPE_ERROR,
        byte_array_type("Type '") + TypeRegister<T>::name() +
        byte_array_type("' differs from type '") + ErrorCodeToMessage(type) +"'");      
 
    }
    Deserialize(*this, val);
    return *this;
  }


  template <typename T>
  TypedByte_ArrayBuffer &Pack(T const &val) {
    return this->operator<<(val);
  }

  template <typename T>
  TypedByte_ArrayBuffer &Unpack(T &val) {
    return this->operator>>(val);
  }
 
  void Seek(std::size_t const &p) { pos_ = p; }
  std::size_t Tell() const { return pos_; }

  std::size_t size() const { return data_.size(); }
  int64_t bytes_left() const { return int64_t(data_.size()) - int64_t(pos_); }    
  byte_array_type const &data() const { return data_; }

 private:
  byte_array_type data_;
  std::size_t pos_ = 0;
};
};
};

#endif
