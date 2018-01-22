#ifndef RPC_CALLABLE_CLASS_MEMBER_HPP
#define RPC_CALLABLE_CLASS_MEMBER_HPP
#include "rpc/abstract_callable.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/typed_byte_array_buffer.hpp"

namespace fetch {
namespace rpc {

/* A member function wrapper that takes a serialized input.
 * @C is the class type to which the member function belongs.
 * @F is the function signature.
 *
 * This module should be benchmarked against the more general class
 * <Function>. If there is no notable perfomance difference this
 * implementation should be dropped to keep the code base small and
 * simple (TODO).
 */
template <typename C, typename F>
class CallableClassMember;


/* A member function wrapper that takes a serialized input.
 * @C is the class type.
 * @R is the type of the return value.
 * @Args are the arguments.
 */
template <typename C, typename R, typename... Args>
class CallableClassMember<C, R(Args...)> : public AbstractCallable {
private:
  typedef R return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)(Args...);
  //< definintion of the member function type.

  /* A struct for invoking the member function once we have
   * unpacked all arguments.
   * @U is the return type.
   * @used_args are the types of the function arguments.
   *
   * This implementation invokes the member function with unpacked
   * arguments and packs the result using the supplied serializer.  
   */
  template< typename U, typename... used_args >
  struct Invoke {
    /* Calls a member function with unpacked arguments.
     * @result is a serializer for storing the result.
     * @cls is the class instance.
     * @m is a pointer to the member function.
     * @used_args are the unpacked arguments.
     */
    static void MemberFunction(serializer_type &result, class_type &cls,
                               member_function_pointer &m,
                               used_args&... args) {
      result << (cls.*m)(args...);
     };
  };

  /* Special case for invocation with return type void.
   * @used_args are the types of the function arguments.
   *
   * In case of void as return type, the result is always 0 packed in a
   * uint8_t.
   */  
   template< typename... used_args >
   struct Invoke<void, used_args...> {
     static void MemberFunction(serializer_type &result, class_type &cls,
                                  member_function_pointer &m,
                                  used_args&... args) {
       result << uint8_t(0);
       (cls.*m)(args...);
     };
     
   };

  /* Struct used for unrolling arguments in a function signature.
   * @used_args are the unpacked arguments.
   */  
  template <typename... used_args>
  struct UnrollArguments {
    /* Struct for loop definition.
     * @T is the type of the next argument to be unrolled.
     * @remaining_args are the arugments which has not yet been unrolled.
     */  
    template <typename T, typename... remaining_args>
    struct LoopOver {
      static void Unroll(serializer_type &result, class_type &cls,
                         member_function_pointer &m, serializer_type &s,
                         used_args &... used) {
        T l;
        s >> l;
        UnrollArguments<used_args..., T>::template LoopOver<
            remaining_args...>::Unroll(result, cls, m, s, used..., l);
      }
    };
    
    /* Struct for loop termination
     * @T is the type of the last argument to be unrolled.
     */  
    template <typename T>
    struct LoopOver<T> {
      static void Unroll(serializer_type &result, class_type &cls,
                         member_function_pointer &m, serializer_type &s,
                         used_args &... used) {
        T l;
        s >> l;
        Invoke<return_type, used_args..., T>::MemberFunction(result, cls, m, used..., l);
      }
    };
  };

 public:
  /* Creates a callable class member.
   * @cls is the class instance.
   * @function is the member function.
   */
  CallableClassMember(class_type *cls, member_function_pointer function) {
    class_ = cls;
    function_ = function;
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
  void operator()(serializer_type &result, serializer_type &params) override {
    UnrollArguments<>::template LoopOver<Args...>::Unroll(
        result, *class_, this->function_, params);
  }

 private:
  class_type *class_;
  member_function_pointer function_;
};


  // No function args
template <typename C, typename R>
class CallableClassMember<C, R()> : public AbstractCallable {
 private:
  typedef R return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)();

 public:
  CallableClassMember(class_type *cls, member_function_pointer value) {
    class_ = cls;
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override {
    result << ( (*class_).*function_)();
  }

 private:
  class_type *class_;
  member_function_pointer function_;
};

  // No function args, void return
template <typename C>
class CallableClassMember<C, void()> : public AbstractCallable {
 private:
  typedef void return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)();

 public:
  CallableClassMember(class_type *cls, member_function_pointer value) {
    class_ = cls;
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override {
    result << 0;
    ( (*class_).*function_)();
  }

 private:
  class_type *class_;
  member_function_pointer function_;
};
  
  
};
};

#endif
