#ifndef SERIALIZER_TYPE_REGISTER_HPP
#define SERIALIZER_TYPE_REGISTER_HPP
#include <string>
#include "serializer/serializable_exception.hpp"
#include "byte_array/const_byte_array.hpp"
namespace fetch {
namespace serializers {

template <typename T>
struct TypeRegister {
  static const byte_array::ConstByteArray name;
  enum { value = 0 };
};

template <typename T>
const byte_array::ConstByteArray TypeRegister<T>::name = "variant";

#define REGISTER_SERIALIZE_TYPE(symbol, type, val)         \
  template <>                                          \
  struct TypeRegister<type> {                          \
    static byte_array::ConstByteArray const name;                           \
    enum { value = uint8_t(val) };                                     \
  };                                                   \
  byte_array::ConstByteArray const TypeRegister<type>::name = #symbol


REGISTER_SERIALIZE_TYPE('double', double, 0);
REGISTER_SERIALIZE_TYPE('float', float, 1);

REGISTER_SERIALIZE_TYPE('u64', uint64_t, 2);
REGISTER_SERIALIZE_TYPE('i64', int64_t, 3);

REGISTER_SERIALIZE_TYPE('u32', uint32_t, 4);
REGISTER_SERIALIZE_TYPE('i32', int32_t, 5);

REGISTER_SERIALIZE_TYPE('u16', uint16_t, 6);
REGISTER_SERIALIZE_TYPE('i16', int16_t, 7);

REGISTER_SERIALIZE_TYPE('u8', uint8_t, 8);
REGISTER_SERIALIZE_TYPE('i8', int8_t, 8);
REGISTER_SERIALIZE_TYPE('i8', char, 8);

REGISTER_SERIALIZE_TYPE('b8', bool, 11);  

REGISTER_SERIALIZE_TYPE('str', byte_array::BasicByteArray, 12);
REGISTER_SERIALIZE_TYPE('str', byte_array::ByteArray,12);
REGISTER_SERIALIZE_TYPE('str', byte_array::ConstByteArray,12);
REGISTER_SERIALIZE_TYPE('str', std::string,12);
REGISTER_SERIALIZE_TYPE('str', char const*,12);
REGISTER_SERIALIZE_TYPE('str', char *,12);

REGISTER_SERIALIZE_TYPE('excep', SerializableException, 13);

#undef REGISTER_SERIALIZE_TYPE
};
};

#endif
