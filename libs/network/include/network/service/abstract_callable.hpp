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

#include "core/byte_array/byte_array.hpp"
#include "core/logging.hpp"
#include "core/serializers/type_register.hpp"
#include "meta/value_util.hpp"
#include "network/service/types.hpp"

#include <cstdint>
#include <string>
#include <type_traits>

namespace fetch {
namespace service {

namespace details {

template <typename T>
using base_type = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename R, typename... Args>
struct ArgsToString
{
  static std::string Value()
  {
    return value_util::Accumulate(
        [](std::string accum, std::string arg) { return std::move(accum) + ", " + arg; },
        serializers::TypeRegister<base_type<Args>>::Value()...);
  }
};

template <typename R>
struct ArgsToString<R>
{
  static std::string Value()
  {
    return "";
  }
};

template <typename R>
struct ArgsToString<R, void> : ArgsToString<R>
{
};

template <typename C, typename R, typename... Args>
struct SignatureToString
{
  static std::string Signature()
  {
    return std::string(serializers::TypeRegister<base_type<R>>::name()) + std::string(" ") +
           std::string(serializers::TypeRegister<base_type<C>>::name()) +
           std::string("::function_pointer") + std::string("(") +
           ArgsToString<R, Args...>::Value() + std::string(")");
  }
};

/* Argument packing routines for callables.
 * @T is the type of the argument that will be packed next.
 * @arguments are the arguments that remains to be pack
 *
 * This struct contains packer structs that takes function arguments and
 * serializes them into a referenced byte array. The content of this
 * struct belongs to the implementational details and are hence not
 * exposed directly to the developer.
 */
template <typename... Args>
struct Packer
{
  /* Implementation of the serialization.
   *
   * @serializer is a reference to the serializer.
   *
   * serializer is always left at position 0.
   */
  template <typename S>
  static void SerializeArguments(S &serializer, Args &&... args)
  {
    value_util::ForEach([&](auto &&arg) { serializer << arg; }, std::forward<Args>(args)...);
    serializer.seek(0);
  }
};

}  // namespace details

/* This function packs a function call into a byte array.
 * @arguments are the argument types of args.
 * @serializer is the serializer to which the arguments will be packed.
 * @protocol is the protocol the call belongs to.
 * @function is the function that is being called.
 * @args are the arguments to the function.
 *
 * For this function to work, it is a requirement that there exists a
 * serilization implementation for all argument types in the argument
 * list.
 *
 * The serializer is always left at position 0.
 */
template <typename S, typename... arguments>
void PackCall(S &serializer, protocol_handler_type const &protocol,
              function_handler_type const &function, arguments &&... args)
{
  serializer << protocol;
  serializer << function;

  details::Packer<arguments...>::SerializeArguments(serializer, std::forward<arguments>(args)...);
}

/* This function packs a function call using packed arguments.
 * @serializer is the serializer to which the arguments will be packed.
 * @protocol is the protocol the call belongs to.
 * @function is the function that is being called.
 * @args is the packed arguments.
 *
 * This function can be used to pack aguments with out the need of
 * vadariac arguments.
 *
 * The serializer is left at position 0.
 */
template <typename S>
void PackCallWithPackedArguments(S &serializer, protocol_handler_type const &protocol,
                                 function_handler_type const &function,
                                 byte_array::ByteArray const &args)
{
  serializer << protocol;
  serializer << function;

  serializer.Allocate(args.size());
  serializer.WriteBytes(args.pointer(), args.size());
  serializer.seek(0);
}

/* Function that packs arguments to serializer.
 * @serializer is the serializer to which the arguments will be packed.
 * @args are the arguments to the function.
 *
 * Serializers for all arguments in the argument list are requried.
 */
template <typename S, typename... arguments>
void PackArgs(S &serializer, arguments &&... args)
{
  details::Packer<arguments...>::SerializeArguments(serializer, std::forward<arguments>(args)...);
}

enum Callable
{
  CLIENT_ID_ARG      = 1ull,
  CLIENT_CONTEXT_ARG = 2ull,
};

struct CallableArgumentType
{
  std::reference_wrapper<std::type_info const> type;
  void *                                       pointer;
};

class CallableArgumentList : public std::vector<CallableArgumentType>
{
public:
  template <typename T>
  void PushArgument(T *value)
  {
    std::vector<CallableArgumentType>::push_back(CallableArgumentType{typeid(T), (void *)value});
  }
};

/* Abstract class for callables.
 *
 * This class defines but a single virtual operator.
 */
class AbstractCallable
{
public:
  explicit AbstractCallable(uint64_t meta_data = 0)
    : meta_data_(meta_data)
  {}

  virtual ~AbstractCallable() = default;

  /* Call operator that implements deserialization and invocation of function.
   * @result is a serializer used to serialize the result.
   * @params is a serializer that is used to deserialize the arguments.
   */
  virtual void operator()(serializer_type &result, serializer_type &params) = 0;
  virtual void operator()(serializer_type &result, CallableArgumentList const &additional_args,
                          serializer_type &params)                          = 0;

  uint64_t meta_data() const
  {
    return meta_data_;
  }
  std::string const &signature() const
  {
    return signature_;
  }

protected:
  void SetSignature(std::string const &signature)
  {
    signature_ = signature;
  }

private:
  uint64_t    meta_data_ = 0;
  std::string signature_;
};
}  // namespace service
}  // namespace fetch
