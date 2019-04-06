#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/serializable_exception.hpp"
#include <string>
#include <typeinfo>
namespace fetch {
namespace serializers {

class CallContext;

template <typename T>
struct TypeRegister
{
  using value_type = uint8_t;
  static constexpr char const *name()
  {
    return typeid(T).name();
  };
  enum
  {
    value = 0
  };
};

template <std::size_t E>
struct TypeErrorRegister
{
  static constexpr char const *name()
  {
    return "unknown";
  };
  enum
  {
    value = 0
  };
};

#define REGISTER_SERIALIZE_SYMBOL_TYPE(symbol, type, val) \
  template <>                                             \
  struct TypeRegister<type>                               \
  {                                                       \
    static constexpr char const *name()                   \
    {                                                     \
      return symbol;                                      \
    };                                                    \
    enum                                                  \
    {                                                     \
      value = uint8_t(val)                                \
    };                                                    \
  }

#define REGISTER_SERIALIZE_TYPE(symbol, type, val) \
  template <>                                      \
  struct TypeRegister<type>                        \
  {                                                \
    static constexpr char const *name()            \
    {                                              \
      return symbol;                               \
    };                                             \
    enum                                           \
    {                                              \
      value = uint8_t(val)                         \
    };                                             \
  };                                               \
                                                   \
  template <>                                      \
  struct TypeErrorRegister<val>                    \
  {                                                \
    static constexpr char const *name()            \
    {                                              \
      return symbol;                               \
    };                                             \
    enum                                           \
    {                                              \
      value = uint8_t(val)                         \
    };                                             \
  }

REGISTER_SERIALIZE_TYPE("double", double, 1);
REGISTER_SERIALIZE_TYPE("float", float, 2);

REGISTER_SERIALIZE_TYPE("uint64_t", uint64_t, 3);
REGISTER_SERIALIZE_TYPE("int64_t", int64_t, 4);

REGISTER_SERIALIZE_TYPE("uint32_t", uint32_t, 5);
REGISTER_SERIALIZE_TYPE("int32_t", int32_t, 6);

REGISTER_SERIALIZE_TYPE("uint16_t", uint16_t, 7);
REGISTER_SERIALIZE_TYPE("int16_t", int16_t, 8);

REGISTER_SERIALIZE_TYPE("uint8_t", uint8_t, 9);
REGISTER_SERIALIZE_SYMBOL_TYPE("int8_t", int8_t, 9);
REGISTER_SERIALIZE_SYMBOL_TYPE("int8_t", char, 9);

REGISTER_SERIALIZE_TYPE("bool", bool, 11);

REGISTER_SERIALIZE_TYPE("str", byte_array::ConstByteArray, 12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", byte_array::ByteArray, 12);
// REGISTER_SERIALIZE_SYMBOL_TYPE("str", byte_array::ConstByteArray, 12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", std::string, 12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", char const *, 12);
REGISTER_SERIALIZE_SYMBOL_TYPE("str", char *, 12);

REGISTER_SERIALIZE_TYPE("excep", SerializableException, 13);
REGISTER_SERIALIZE_TYPE("contextp", CallContext const *, 14);

#undef REGISTER_SERIALIZE_TYPE
#undef REGISTER_SERIALIZE_SYMBOL_TYPE
inline byte_array::ConstByteArray ErrorCodeToMessage(std::size_t n)
{
  switch (n)
  {
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
}  // namespace serializers
}  // namespace fetch
