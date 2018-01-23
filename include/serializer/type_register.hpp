#ifndef SERIALIZER_TYPE_REGISTER_HPP
#define SERIALIZER_TYPE_REGISTER_HPP
#include <string>
#include "byte_array/const_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

namespace fetch {
namespace serializers {

template <typename T>
struct TypeRegister {
  static const byte_array::ConstByteArray name;
};

template <typename T>
const byte_array::ConstByteArray TypeRegister<T>::name = "variant";

#define REGISTER_SERIALIZE_TYPE(symbol, type)          \
  template <>                                          \
  struct TypeRegister<type> {                          \
    static byte_array::ConstByteArray const name; \
  };                                                   \
  byte_array::ConstByteArray const TypeRegister<type>::name = #symbol

REGISTER_SERIALIZE_TYPE('u64', uint64_t);
REGISTER_SERIALIZE_TYPE('i64', int64_t);

REGISTER_SERIALIZE_TYPE('u32', uint32_t);
REGISTER_SERIALIZE_TYPE('i32', int32_t);

REGISTER_SERIALIZE_TYPE('u16', uint16_t);
REGISTER_SERIALIZE_TYPE('i16', int16_t);

REGISTER_SERIALIZE_TYPE('u8', uint8_t);
REGISTER_SERIALIZE_TYPE('i8', int8_t);
REGISTER_SERIALIZE_TYPE('i8', char);

REGISTER_SERIALIZE_TYPE('b8', bool);  

REGISTER_SERIALIZE_TYPE('str', byte_array::ConstByteArray);
REGISTER_SERIALIZE_TYPE('str', std::string);
REGISTER_SERIALIZE_TYPE('str', char const*);
REGISTER_SERIALIZE_TYPE('str', char *);

REGISTER_SERIALIZE_TYPE('excep', SerializableException);

#undef REGISTER_SERIALIZE_TYPE
};
};

#endif
