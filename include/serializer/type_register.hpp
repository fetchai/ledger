#ifndef SERIALIZER_TYPE_REGISTER_HPP
#define SERIALIZER_TYPE_REGISTER_HPP

#include "serializer/serializable_exception.hpp"
#include "byte_array/const_byte_array.hpp"
#include <string>
#include<typeinfo>
namespace fetch {
namespace serializers {

template <typename T>
struct TypeRegister {
  typedef uint8_t value_type;  
  static constexpr char const* name() {
    return typeid(T).name();
  };
  enum { value = 0 };
};

template <std::size_t E>
struct TypeErrorRegister {
  static constexpr char const* name() {
    return "unknown";
  };  
  enum { value = 0 };
};

    
#define REGISTER_SERIALIZE_SYMBOL_TYPE(symbol, type, val)                      \
  template <>                                                           \
  struct TypeRegister<type> {                                           \
    static constexpr char const* name() {                               \
      return symbol;                                                    \
    };                                                                  \
    enum { value = uint8_t(val) };                                      \
  }                                  


#define REGISTER_SERIALIZE_TYPE(symbol, type, val)               \
  template <>                                                           \
  struct TypeRegister<type> {                                           \
    static constexpr char const* name() {                               \
      return symbol;                                                    \
    };                                                                  \
    enum { value = uint8_t(val) };                                      \
  };                                                                    \
                                                                        \
  template <>                                                           \
  struct TypeErrorRegister<val> {                                       \
    static constexpr char const* name() {                               \
      return symbol;                                                    \
    };                                                                  \
    enum { value = uint8_t(val) };                                      \
  }                                 
    


REGISTER_SERIALIZE_TYPE("double", double, 1);
REGISTER_SERIALIZE_TYPE("float", float, 2);

REGISTER_SERIALIZE_TYPE("u64", uint64_t, 3);
REGISTER_SERIALIZE_TYPE("i64", int64_t, 4);

REGISTER_SERIALIZE_TYPE("u32", uint32_t, 5);
REGISTER_SERIALIZE_TYPE("i32", int32_t, 6);

REGISTER_SERIALIZE_TYPE("u16", uint16_t, 7);
REGISTER_SERIALIZE_TYPE("i16", int16_t, 8);

REGISTER_SERIALIZE_TYPE("u8", uint8_t, 9);
REGISTER_SERIALIZE_SYMBOL_TYPE("i8", int8_t, 9);
REGISTER_SERIALIZE_SYMBOL_TYPE("i8", char, 9);

REGISTER_SERIALIZE_TYPE("b8", bool, 11);  

REGISTER_SERIALIZE_TYPE("str", byte_array::BasicByteArray, 12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", byte_array::ByteArray,12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", byte_array::ConstByteArray,12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", std::string,12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", char const*,12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", char *,12);

REGISTER_SERIALIZE_TYPE("excep", SerializableException, 13);

#undef REGISTER_SERIALIZE_TYPE
#undef REGISTER_SERIALIZE_SYMBOL_TYPE
byte_array::ConstByteArray ErrorCodeToMessage(std::size_t n) 
{
  switch(n) {
  case 0:
    return TypeErrorRegister<0>::name();
  case 1:
    return TypeErrorRegister<1>::name();
  case 2:
    return TypeErrorRegister<2>::name();
  case 3:
    return TypeErrorRegister<3>::name();
  case 4:
    return TypeErrorRegister<4>::name();
  case 5:
    return TypeErrorRegister<5>::name();
  case 6:
    return TypeErrorRegister<6>::name();
  case 7:
    return TypeErrorRegister<7>::name();
  case 8:
    return TypeErrorRegister<8>::name();
  case 9:
    return TypeErrorRegister<9>::name();
  case 10:
    return TypeErrorRegister<10>::name();
  case 11:
    return TypeErrorRegister<11>::name();
  case 12:
    return TypeErrorRegister<12>::name();
  case 13:
    return TypeErrorRegister<13>::name();
  case 14:
    return TypeErrorRegister<14>::name();
  case 15:
    return TypeErrorRegister<15>::name();        
  }
  
  return "variant";  
}


  
}
}

#endif
