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
#include "core/logger.hpp"
#include "core/serializers/type_register.hpp"
#include "network/service/types.hpp"

namespace fetch {
namespace service {

namespace details {

template <typename T>
using base_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <typename R, typename F, typename... Args>
struct ArgsToString
{
  static std::string Value()
  {
    return serializers::TypeRegister<base_type<F>>::name() + std::string(", ") +
           ArgsToString<R, Args...>::Value();
  }
};

template <typename R, typename F>
struct ArgsToString<R, F>
{
  static std::string Value()
  {
    return serializers::TypeRegister<base_type<F>>::name();
  }
};

template <typename R>
struct ArgsToString<R, void>
{
  static std::string Value()
  {
    return "";
  }
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
template <typename T, typename... arguments>
struct Packer
{
  /* Implementation of the serialization.
   *
   * @serializer is a reference to the serializer.
   *  @next is the next argument that will be fed into the serializer.
   */
  template <typename S>
  static void SerializeArguments(S &serializer, T &&next, arguments &&... args)
  {

    serializer << next;
    Packer<arguments...>::SerializeArguments(serializer, std::forward<arguments>(args)...);
  }
};

/* Termination of the argument packing routines.
 *
 * This specialisation is invoked when only one argument is left.
 */
template <typename T>
struct Packer<T>
{
  /* Implementation of the serialization.
   *
   * @serializer is a reference to the serializer.
   * @last is the last argument that will be fed into the serializer.
   */
  template <typename S>
  static void SerializeArguments(S &serializer, T &&last)
  {
    serializer << last;
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
 * The serializer is is always left at position 0.
 */
template <typename S, typename... arguments>
void PackCall(S &serializer, protocol_handler_type const &protocol,
              function_handler_type const &function, arguments &&... args)
{

  LOG_STACK_TRACE_POINT;

  serializer << protocol;
  serializer << function;

  details::Packer<arguments...>::SerializeArguments(serializer, std::forward<arguments>(args)...);
}

/* This function is the no-argument packer.
 * @serializer is the serializer to which the arguments will be packed.
 * @protocol is the protocol the call belongs to.
 * @function is the function that is being called.
 *
 * This function covers the case where no arguments are given. The
 * serializer is is always left at position 0.
 */
template <typename S>
void PackCall(S &serializer, protocol_handler_type const &protocol,
              function_handler_type const &function)
{
  LOG_STACK_TRACE_POINT;

  serializer << protocol;
  serializer << function;
  serializer.seek(0);
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
  LOG_STACK_TRACE_POINT;

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

/* This is the no-argument packer.
 * @serializer is the serializer to which the arguments will be packed.
 *
 * This function covers the case where no arguments are given. The
 * serializer is is always left at position 0.
 */
template <typename S>
void PackArgs(S &serializer)
{
  LOG_STACK_TRACE_POINT;

  serializer.seek(0);
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

  CallableArgumentType const &operator[](std::size_t n) const
  {
    return std::vector<CallableArgumentType>::operator[](n);
  }
  CallableArgumentType &operator[](std::size_t n)
  {
    return std::vector<CallableArgumentType>::operator[](n);
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
