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

#include "core/serializers/base_types.hpp"
#include "network/service/abstract_callable.hpp"

#include <functional>

namespace fetch {
namespace service {

/* A function wrapper that takes a serialized input.
 * @F is the function signature.
 */
template <typename F>
class Function;

/* A function wrapper that takes a serialized input.
 * @R is the type of the return value.
 * @Args are the arguments.
 */
template <typename R, typename... Args>
class Function<R(Args...)> : public AbstractCallable
{
  static constexpr char const *LOGGING_NAME = "Function<R(Args...)>";

private:
  using ReturnType   = R;
  using FunctionType = std::function<R(Args...)>;

  /* A struct for invoking the function once we have unpacked all
   * arguments.
   * @U is the return type.
   * @UsedArgs are the types of the function arguments.
   *
   * This implementation invokes the function with unpacked arguments
   * and packs the result using the supplied serializer.
   */
  template <typename U, typename... UsedArgs>
  struct Invoke
  {
    static void MemberFunction(SerializerType &result, FunctionType &m, UsedArgs &... args)
    {
      result << ReturnType(m(args...));
    };
  };

  /* Special case for invocation with return type void.
   * @UsedArgs are the types of the function arguments.
   *
   * In case of void as return type, the result is always 0 packed in a
   * uint8_t.
   */
  template <typename... UsedArgs>
  struct Invoke<void, UsedArgs...>
  {
    static void MemberFunction(SerializerType &result, FunctionType &m, UsedArgs &... args)
    {
      m(args...);
      result << uint8_t(0);
    };
  };

  /* Struct used for unrolling arguments in a function signature.
   * @UsedArgs are the unpacked arguments.
   */
  template <typename... UsedArgs>
  struct UnrollArguments
  {
    /* Struct for loop definition.
     * @T is the type of the next argument to be unrolled.
     * @remaining_args are the arugments which has not yet been unrolled.
     */
    template <typename T, typename... remaining_args>
    struct LoopOver
    {
      static void Unroll(SerializerType &result, FunctionType &m, SerializerType &s,
                         UsedArgs &... used)
      {
        T l;
        s >> l;
        UnrollArguments<UsedArgs..., T>::template LoopOver<remaining_args...>::Unroll(result, m, s,
                                                                                      used..., l);
      }
    };

    /* Struct for loop termination
     * @T is the type of the last argument to be unrolled.
     */
    template <typename T>
    struct LoopOver<T>
    {
      static void Unroll(SerializerType &result, FunctionType &m, SerializerType &s,
                         UsedArgs &... used)
      {
        T l;
        s >> l;
        Invoke<ReturnType, UsedArgs..., T>::MemberFunction(result, m, used..., l);
      }
    };
  };

public:
  /* Creates a function with serialized arguments.
   * @function is the member function.
   */
  Function(FunctionType value)
    : function_{std::move(value)}
  {}

  /* Operator to invoke the function.
   * @result is the serializer to which the result is written.
   * @params is a seralizer containing the function parameters.
   *
   * Note that the parameter seralizer can container more information
   * than just the function arguments. It is therefore a requirement
   * that the serializer is positioned at the beginning of the argument
   * list.
   */
  void operator()(SerializerType &result, SerializerType &params) override
  {
    UnrollArguments<>::template LoopOver<Args...>::Unroll(result, function_, params);
  }
  void operator()(SerializerType & /*result*/, CallableArgumentList const & /*additional_args*/,
                  SerializerType & /*params*/) override
  {
    TODO_FAIL("No support for custom added args yet");
  }

private:
  FunctionType function_;
};

// No function args
template <typename R>
class Function<R()> : public AbstractCallable
{
private:
  using ReturnType   = R;
  using FunctionType = std::function<R()>;

public:
  static constexpr char const *LOGGING_NAME = "Function<R()>";

  Function(FunctionType value)
    : function_{std::move(value)}
  {}

  void operator()(SerializerType &result, SerializerType & /*params*/) override
  {
    result << R(function_());
  }

  void operator()(SerializerType & /*result*/, CallableArgumentList const & /*additional_args*/,
                  SerializerType & /*params*/) override
  {
    TODO_FAIL("No support for custom added args yet");
  }

private:
  FunctionType function_;
};

// No function args, void return
template <>
class Function<void()> : public AbstractCallable
{
private:
  using ReturnType   = void;
  using FunctionType = std::function<void()>;

public:
  static constexpr char const *LOGGING_NAME = "Function<void()>";

  Function(FunctionType value)
    : function_{std::move(value)}
  {}

  void operator()(SerializerType &result, SerializerType & /*params*/) override
  {
    function_();
    result << uint8_t(0);
  }
  void operator()(SerializerType & /*result*/, CallableArgumentList const & /*additional_args*/,
                  SerializerType & /*params*/) override
  {
    TODO_FAIL("No support for custom added args yet");
  }

private:
  FunctionType function_;
};
}  // namespace service
}  // namespace fetch
