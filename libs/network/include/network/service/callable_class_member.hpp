#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/serializers/base_types.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "logging/logging.hpp"
#include "network/service/abstract_callable.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <typeinfo>

namespace fetch {
namespace service {

namespace details {
template <typename... Types>
struct CountArguments
{
  static const std::size_t value = sizeof...(Types);
};

/* A struct for invoking the member function once we have
 * unpacked all arguments.
 * @U is the return type.
 * @UsedArgs are the types of the function arguments.
 *
 * This implementation invokes the member function with unpacked
 * arguments and packs the result using the supplied serializer.
 */
template <typename ClassType, typename MemberFunctionPointer, typename U, typename... UsedArgs>
struct Invoke
{
  /* Calls a member function with unpacked arguments.
   * @result is a serializer for storing the result.
   * @cls is the class instance.
   * @m is a pointer to the member function.
   * @UsedArgs are the unpacked arguments.
   */
  static void MemberFunction(SerializerType &result, ClassType &cls, MemberFunctionPointer &m,
                             UsedArgs &... args)
  {
    auto                     ret = (cls.*m)(args...);
    serializers::SizeCounter counter;
    counter << ret;

    result.Reserve(counter.size());
    result << ret;
  };
};

/* Special case for invocation with return type void.
 * @UsedArgs are the types of the function arguments.
 *
 * In case of void as return type, the result is always 0 packed in a
 * uint8_t.
 */
template <typename ClassType, typename MemberFunctionPointer, typename... UsedArgs>
struct Invoke<ClassType, MemberFunctionPointer, void, UsedArgs...>
{
  static void MemberFunction(SerializerType &result, ClassType &cls, MemberFunctionPointer &m,
                             UsedArgs &... args)
  {
    result << uint8_t(0);
    (cls.*m)(args...);
  };
};

/* Struct used for unrolling arguments in a function signature.
 * @UsedArgs are the unpacked arguments.
 */
template <typename ClassType, typename MemberFunctionPointer, typename ReturnType,
          typename... UsedArgs>
struct UnrollArguments
{
  /* Struct for loop definition.
   * @T is the type of the next argument to be unrolled.
   * @remaining_args are the arguments which has not yet been unrolled.
   */
  template <std::size_t R, typename... remaining_args>
  struct LoopOver;

  template <std::size_t R, typename T, typename... remaining_args>
  struct LoopOver<R, T, remaining_args...>
  {
    static void Unroll(SerializerType &result, ClassType &cls, MemberFunctionPointer &m,
                       SerializerType &s, UsedArgs &... used)
    {
      std::decay_t<T> l;

      s >> l;
      UnrollArguments<ClassType, MemberFunctionPointer, ReturnType, UsedArgs..., T>::
          template LoopOver<CountArguments<remaining_args...>::value, remaining_args...>::Unroll(
              result, cls, m, s, used..., l);
    }
  };

  /* Struct for loop termination
   * @T is the type of the last argument to be unrolled.
   */
  template <std::size_t R, typename T>
  struct LoopOver<R, T>
  {
    static void Unroll(SerializerType &result, ClassType &cls, MemberFunctionPointer &m,
                       SerializerType &s, UsedArgs &... used)
    {
      std::decay_t<T> l;

      s >> l;
      Invoke<ClassType, MemberFunctionPointer, ReturnType, UsedArgs..., T>::MemberFunction(
          result, cls, m, used..., l);
    }
  };

  template <std::size_t R>
  struct LoopOver<R>
  {
    static void Unroll(SerializerType &result, ClassType &cls, MemberFunctionPointer &m,
                       SerializerType & /*s*/, UsedArgs &... used)
    {
      assert(R == 0);

      Invoke<ClassType, MemberFunctionPointer, ReturnType, UsedArgs...>::MemberFunction(result, cls,
                                                                                        m, used...);
    }
  };
};

template <std::size_t COUNTER, typename ClassType, typename MemberFunctionPointer,
          typename ReturnType, typename... UsedArgs>
struct UnrollPointers
{
  template <typename T, typename... remaining_args>
  struct LoopOver
  {
    static void Unroll(SerializerType &result, ClassType &cls, MemberFunctionPointer &m,
                       CallableArgumentList const &additional_args, SerializerType &s,
                       UsedArgs &... used)
    {
      assert(COUNTER - 1 < additional_args.size());
      auto const &arg = additional_args[COUNTER - 1];

      if (typeid(T) != arg.type.get())
      {
        // TODO(issue 11): Make serializable
        throw std::runtime_error(std::string("argument type mismatch for Callabale.") +
                                 typeid(T).name() + " != " + arg.type.get().name() +
                                 " TODO: Make custom " + "exception");
      }

      auto ptr = static_cast<std::decay_t<T> *>(arg.pointer);
      UnrollPointers<COUNTER - 1, ClassType, MemberFunctionPointer, ReturnType, UsedArgs...,
                     T>::template LoopOver<remaining_args...>::Unroll(result, cls, m,
                                                                      additional_args, s, used...,
                                                                      *ptr);
    }
  };
};

template <typename ClassType, typename MemberFunctionPointer, typename ReturnType,
          typename... UsedArgs>
struct UnrollPointers<0, ClassType, MemberFunctionPointer, ReturnType, UsedArgs...>
{
  template <typename... remaining_args>
  struct LoopOver
  {
    static void Unroll(SerializerType &result, ClassType &cls, MemberFunctionPointer &m,
                       CallableArgumentList const & /*additional_args*/, SerializerType &s,
                       UsedArgs &... used)
    {
      UnrollArguments<ClassType, MemberFunctionPointer, ReturnType, UsedArgs...>::template LoopOver<
          CountArguments<remaining_args...>::value, remaining_args...>::Unroll(result, cls, m, s,
                                                                               used...);
    }
  };
};
}  // namespace details

/* A member function wrapper that takes a serialized input.
 * @C is the class type to which the member function belongs.
 * @F is the function signature.
 *
 * This module should be benchmarked against the more general class
 * <Function>. If there is no notable performance difference this
 * implementation should be dropped to keep the code base small and
 * simple.
 *
 * TODO(issue 23):
 */
template <typename C, typename F, std::size_t N = 0>
class CallableClassMember;

/* A member function wrapper that takes a serialized input.
 * @C is the class type.
 * @R is the type of the return value.
 * @Args are the arguments.
 */
template <typename C, typename R, typename... Args, std::size_t N>
class CallableClassMember<C, R(Args...), N> : public AbstractCallable
{
private:
  using ReturnType            = R;
  using ClassType             = C;
  using MemberFunctionPointer = ReturnType (ClassType::*)(Args...);
  //< definition of the member function type.

public:
  enum
  {
    EXTRA_ARGS = N
  };

  /* Creates a callable class member.
   * @cls is the class instance.
   * @function is the member function.
   */
  CallableClassMember(ClassType *cls, MemberFunctionPointer function)
  {
    class_    = cls;
    function_ = function;
    this->SetSignature(details::SignatureToString<C, R, Args...>::Signature());
  }

  CallableClassMember(uint64_t arguments, ClassType *cls, MemberFunctionPointer value)
    : AbstractCallable(arguments)
  {
    class_    = cls;
    function_ = value;
    this->SetSignature(details::SignatureToString<C, R, Args...>::Signature());
  }

  /* Operator to invoke the function.
   * @result is the serializer to which the result is written.
   * @params is a serializer containing the function parameters.
   *
   * Note that the parameter serializer can contain more information
   * than just the function arguments. It is therefore a requirement
   * that the serializer is positioned at the beginning of the argument
   * list.
   */
  void operator()(SerializerType &result, SerializerType &params) override
  {
    details::UnrollArguments<ClassType, MemberFunctionPointer, ReturnType>::template LoopOver<
        details::CountArguments<Args...>::value, Args...>::Unroll(result, *class_, this->function_,
                                                                  params);
  }

  void operator()(SerializerType &result, CallableArgumentList const &additional_args,
                  SerializerType &params) override
  {
    detailed_assert(EXTRA_ARGS == additional_args.size());

    details::UnrollPointers<N, ClassType, MemberFunctionPointer, ReturnType>::template LoopOver<
        Args...>::Unroll(result, *class_, this->function_, additional_args, params);
  }

private:
  ClassType *           class_;
  MemberFunctionPointer function_;
};

// No function args
template <typename C, typename R>
class CallableClassMember<C, R(), 0> : public AbstractCallable
{
private:
  using ReturnType            = R;
  using ClassType             = C;
  using MemberFunctionPointer = ReturnType (ClassType::*)();

public:
  CallableClassMember(ClassType *cls, MemberFunctionPointer value)
  {
    class_    = cls;
    function_ = value;
  }

  CallableClassMember(uint64_t arguments, ClassType *cls, MemberFunctionPointer value)
    : AbstractCallable(arguments)
  {
    class_    = cls;
    function_ = value;
  }

  void operator()(SerializerType &result, SerializerType & /*params*/) override
  {
    auto ret = ((*class_).*function_)();

    serializers::SizeCounter counter;
    counter << ret;
    result.Reserve(counter.size());
    result << ret;
  }

  void operator()(SerializerType &result, CallableArgumentList const & /*additional_args*/,
                  SerializerType & /*params*/) override
  {
    auto                     ret = ((*class_).*function_)();
    serializers::SizeCounter counter;
    counter << ret;
    result.Reserve(counter.size());
    result << ret;
  }

private:
  ClassType *           class_;
  MemberFunctionPointer function_;
};

// No function args, void return
template <typename C>
class CallableClassMember<C, void(), 0> : public AbstractCallable
{
private:
  using ReturnType            = void;
  using ClassType             = C;
  using MemberFunctionPointer = ReturnType (ClassType::*)();

public:
  CallableClassMember(ClassType *cls, MemberFunctionPointer value)
  {
    class_    = cls;
    function_ = value;
  }

  CallableClassMember(uint64_t arguments, ClassType *cls, MemberFunctionPointer value)
    : AbstractCallable(arguments)
  {
    class_    = cls;
    function_ = value;
  }

  void operator()(SerializerType &result, SerializerType & /*params*/) override
  {
    result << 0;
    ((*class_).*function_)();
  }

  void operator()(SerializerType &result, CallableArgumentList const & /*additional_args*/,
                  SerializerType & /*params*/) override
  {
    result << 0;
    ((*class_).*function_)();
  }

private:
  ClassType *           class_;
  MemberFunctionPointer function_;
};
}  // namespace service
}  // namespace fetch
