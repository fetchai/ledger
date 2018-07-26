#pragma once
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"
#include "network/service/abstract_callable.hpp"
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
 * @used_args are the types of the function arguments.
 *
 * This implementation invokes the member function with unpacked
 * arguments and packs the result using the supplied serializer.
 */
template <typename class_type, typename member_function_pointer, typename U,
          typename... used_args>
struct Invoke
{
  /* Calls a member function with unpacked arguments.
   * @result is a serializer for storing the result.
   * @cls is the class instance.
   * @m is a pointer to the member function.
   * @used_args are the unpacked arguments.
   */
  static void MemberFunction(serializer_type &result, class_type &cls,
                             member_function_pointer &m, used_args &... args)
  {
    auto                                      ret = (cls.*m)(args...);
    serializers::SizeCounter<serializer_type> counter;
    counter << ret;

    result.Reserve(counter.size());
    result << ret;
  };
};

/* Special case for invocation with return type void.
 * @used_args are the types of the function arguments.
 *
 * In case of void as return type, the result is always 0 packed in a
 * uint8_t.
 */
template <typename class_type, typename member_function_pointer,
          typename... used_args>
struct Invoke<class_type, member_function_pointer, void, used_args...>
{
  static void MemberFunction(serializer_type &result, class_type &cls,
                             member_function_pointer &m, used_args &... args)
  {
    result << uint8_t(0);
    (cls.*m)(args...);
  };
};

/* Struct used for unrolling arguments in a function signature.
 * @used_args are the unpacked arguments.
 */
template <typename class_type, typename member_function_pointer,
          typename return_type, typename... used_args>
struct UnrollArguments
{
  /* Struct for loop definition.
   * @T is the type of the next argument to be unrolled.
   * @remaining_args are the arugments which has not yet been unrolled.
   */
  template <std::size_t R, typename... remaining_args>
  struct LoopOver;

  template <std::size_t R, typename T, typename... remaining_args>
  struct LoopOver<R, T, remaining_args...>
  {
    static void Unroll(serializer_type &result, class_type &cls,
                       member_function_pointer &m, serializer_type &s,
                       used_args &... used)
    {
      typename std::decay<T>::type l;
      s >> l;
      UnrollArguments<
          class_type, member_function_pointer, return_type, used_args...,
          T>::template LoopOver<CountArguments<remaining_args...>::value,
                                remaining_args...>::Unroll(result, cls, m, s,
                                                           used..., l);
    }
  };

  /* Struct for loop termination
   * @T is the type of the last argument to be unrolled.
   */
  template <std::size_t R, typename T>
  struct LoopOver<R, T>
  {
    static void Unroll(serializer_type &result, class_type &cls,
                       member_function_pointer &m, serializer_type &s,
                       used_args &... used)
    {
      typename std::decay<T>::type l;

      s >> l;
      Invoke<class_type, member_function_pointer, return_type, used_args...,
             T>::MemberFunction(result, cls, m, used..., l);
    }
  };

  template <std::size_t R>
  struct LoopOver<R>
  {
    static void Unroll(serializer_type &result, class_type &cls,
                       member_function_pointer &m, serializer_type &s,
                       used_args &... used)
    {
      assert(R == 0);

      Invoke<class_type, member_function_pointer, return_type,
             used_args...>::MemberFunction(result, cls, m, used...);
    }
  };
};

template <std::size_t COUNTER, typename class_type,
          typename member_function_pointer, typename return_type,
          typename... used_args>
struct UnrollPointers
{
  template <typename T, typename... remaining_args>
  struct LoopOver
  {
    static void Unroll(serializer_type &result, class_type &cls,
                       member_function_pointer &   m,
                       CallableArgumentList const &additional_args,
                       serializer_type &           s, used_args &... used)
    {
      assert(COUNTER - 1 < additional_args.size());
      auto const &arg = additional_args[COUNTER - 1];

      if (typeid(T) != arg.type.get())
      {
        // TODO: Make serializable
        throw std::runtime_error(
            "argument type mismatch for Callabale. TODO: Make custom "
            "exception");
      }

      typename std::decay<T>::type *ptr =
          (typename std::decay<T>::type *)arg.pointer;
      UnrollPointers<COUNTER - 1, class_type, member_function_pointer,
                     return_type, used_args..., T>::
          template LoopOver<remaining_args...>::Unroll(
              result, cls, m, additional_args, s, used..., *ptr);
    }
  };
};

template <typename class_type, typename member_function_pointer,
          typename return_type, typename... used_args>
struct UnrollPointers<0, class_type, member_function_pointer, return_type,
                      used_args...>
{
  template <typename... remaining_args>
  struct LoopOver
  {
    static void Unroll(serializer_type &result, class_type &cls,
                       member_function_pointer &   m,
                       CallableArgumentList const &additional_args,
                       serializer_type &           s, used_args &... used)
    {
      UnrollArguments<class_type, member_function_pointer, return_type,
                      used_args...>::
          template LoopOver<CountArguments<remaining_args...>::value,
                            remaining_args...>::Unroll(result, cls, m, s,
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
 * <Function>. If there is no notable perfomance difference this
 * implementation should be dropped to keep the code base small and
 * simple (TODO).
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
  typedef R return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)(Args...);
  //< definintion of the member function type.

public:
  enum
  {
    EXTRA_ARGS = N
  };

  /* Creates a callable class member.
   * @cls is the class instance.
   * @function is the member function.
   */
  CallableClassMember(class_type *cls, member_function_pointer function)
  {
    LOG_STACK_TRACE_POINT;

    class_    = cls;
    function_ = function;
  }

  CallableClassMember(uint64_t arguments, class_type *cls,
                      member_function_pointer value)
      : AbstractCallable(arguments)
  {
    LOG_STACK_TRACE_POINT;

    class_    = cls;
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

    details::UnrollArguments<class_type, member_function_pointer, return_type>::
        template LoopOver<details::CountArguments<Args...>::value,
                          Args...>::Unroll(result, *class_, this->function_,
                                           params);
  }

  void operator()(serializer_type &           result,
                  CallableArgumentList const &additional_args,
                  serializer_type &           params) override
  {
    LOG_STACK_TRACE_POINT;

    detailed_assert(EXTRA_ARGS == additional_args.size());

    details::UnrollPointers<
        N, class_type, member_function_pointer,
        return_type>::template LoopOver<Args...>::Unroll(result, *class_,
                                                         this->function_,
                                                         additional_args,
                                                         params);
  }

private:
  class_type *            class_;
  member_function_pointer function_;
};

// No function args
template <typename C, typename R>
class CallableClassMember<C, R(), 0> : public AbstractCallable
{
private:
  typedef R return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)();

public:
  CallableClassMember(class_type *cls, member_function_pointer value)
  {
    LOG_STACK_TRACE_POINT;

    class_    = cls;
    function_ = value;
  }

  CallableClassMember(uint64_t arguments, class_type *cls,
                      member_function_pointer value)
      : AbstractCallable(arguments)
  {
    LOG_STACK_TRACE_POINT;

    class_    = cls;
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override
  {
    LOG_STACK_TRACE_POINT;

    auto                                      ret = ((*class_).*function_)();
    serializers::SizeCounter<serializer_type> counter;
    counter << ret;
    result.Reserve(counter.size());
    result << ret;
  }

  void operator()(serializer_type &           result,
                  CallableArgumentList const &additional_args,
                  serializer_type &           params) override
  {
    LOG_STACK_TRACE_POINT;

    auto                                      ret = ((*class_).*function_)();
    serializers::SizeCounter<serializer_type> counter;
    counter << ret;
    result.Reserve(counter.size());
    result << ret;
  }

private:
  class_type *            class_;
  member_function_pointer function_;
};

// No function args, void return
template <typename C>
class CallableClassMember<C, void(), 0> : public AbstractCallable
{
private:
  typedef void return_type;
  typedef C    class_type;
  typedef return_type (class_type::*member_function_pointer)();

public:
  CallableClassMember(class_type *cls, member_function_pointer value)
  {
    LOG_STACK_TRACE_POINT;

    class_    = cls;
    function_ = value;
  }

  CallableClassMember(uint64_t arguments, class_type *cls,
                      member_function_pointer value)
      : AbstractCallable(arguments)
  {
    LOG_STACK_TRACE_POINT;

    class_    = cls;
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override
  {
    LOG_STACK_TRACE_POINT;

    result << 0;
    ((*class_).*function_)();
  }

  void operator()(serializer_type &           result,
                  CallableArgumentList const &additional_args,
                  serializer_type &           params) override
  {
    LOG_STACK_TRACE_POINT;

    result << 0;
    ((*class_).*function_)();
  }

private:
  class_type *            class_;
  member_function_pointer function_;
};
}  // namespace service
}  // namespace fetch
