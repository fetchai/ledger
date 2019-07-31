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
  using return_type   = R;
  using function_type = std::function<R(Args...)>;

  /* A struct for invoking the function once we have unpacked all
   * arguments.
   * @U is the return type.
   * @used_args are the types of the function arguments.
   *
   * This implementation invokes the function with unpacked arguments
   * and packs the result using the supplied serializer.
   */
  template <typename U, typename... used_args>
  struct Invoke
  {
    static void MemberFunction(serializer_type &result, function_type &m, used_args &... args)
    {
      result << return_type(m(args...));
    };
  };

  /* Special case for invocation with return type void.
   * @used_args are the types of the function arguments.
   *
   * In case of void as return type, the result is always 0 packed in a
   * uint8_t.
   */
  template <typename... used_args>
  struct Invoke<void, used_args...>
  {
    static void MemberFunction(serializer_type &result, function_type &m, used_args &... args)
    {
      result << uint8_t(0);
      m(args...);
    };
  };

  /* Struct used for unrolling arguments in a function signature.
   * @used_args are the unpacked arguments.
   */
  template <typename... used_args>
  struct UnrollArguments
  {
    /* Struct for loop definition.
     * @T is the type of the next argument to be unrolled.
     * @remaining_args are the arugments which has not yet been unrolled.
     */
    template <typename T, typename... remaining_args>
    struct LoopOver
    {
      static void Unroll(serializer_type &result, function_type &m, serializer_type &s,
                         used_args &... used)
      {
        T l;
        s >> l;
        UnrollArguments<used_args..., T>::template LoopOver<remaining_args...>::Unroll(result, m, s,
                                                                                       used..., l);
      }
    };

    /* Struct for loop termination
     * @T is the type of the last argument to be unrolled.
     */
    template <typename T>
    struct LoopOver<T>
    {
      static void Unroll(serializer_type &result, function_type &m, serializer_type &s,
                         used_args &... used)
      {
        T l;
        s >> l;
        Invoke<return_type, used_args..., T>::MemberFunction(result, m, used..., l);
      }
    };
  };

public:
  /* Creates a function with serialized arguments.
   * @function is the member function.
   */
  Function(function_type value)
  {
    LOG_STACK_TRACE_POINT;

    function_ = value;
  }

  /* Operator to invoke the function.
   * @result is the serializer to which the result is written.
   * @params is a seralizer containing the function parameters.
   *
   * Note that the parameter seralizer can container more information
   * than just the function arguments. It is therefore a requirement
   * that the serializer is positioned at the beginning of the argument
   * list.
   */
  void operator()(serializer_type &result, serializer_type &params) override
  {
    LOG_STACK_TRACE_POINT;

    UnrollArguments<>::template LoopOver<Args...>::Unroll(result, this->function_, params);
  }
  void operator()(serializer_type & /*result*/, CallableArgumentList const & /*additional_args*/,
                  serializer_type & /*params*/) override
  {
    TODO_FAIL("No support for custom added args yet");
  }

private:
  function_type function_;
};

// No function args
template <typename R>
class Function<R()> : public AbstractCallable
{
private:
  using return_type   = R;
  using function_type = std::function<R()>;

public:
  static constexpr char const *LOGGING_NAME = "Function<R()>";

  Function(function_type value)
  {
    LOG_STACK_TRACE_POINT;

    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type & /*params*/) override
  {
    LOG_STACK_TRACE_POINT;

    result << R(function_());
  }

  void operator()(serializer_type & /*result*/, CallableArgumentList const & /*additional_args*/,
                  serializer_type & /*params*/) override
  {
    TODO_FAIL("No support for custom added args yet");
  }

private:
  function_type function_;
};

// No function args, void return
template <>
class Function<void()> : public AbstractCallable
{
private:
  using return_type   = void;
  using function_type = std::function<void()>;

public:
  static constexpr char const *LOGGING_NAME = "Function<void()>";

  Function(function_type value)
  {
    LOG_STACK_TRACE_POINT;
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type & /*params*/) override
  {
    LOG_STACK_TRACE_POINT;
    result << 0;
    function_();
  }
  void operator()(serializer_type & /*result*/, CallableArgumentList const & /*additional_args*/,
                  serializer_type & /*params*/) override
  {
    TODO_FAIL("No support for custom added args yet");
  }

private:
  function_type function_;
};
}  // namespace service
}  // namespace fetch
