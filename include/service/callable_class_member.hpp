#ifndef SERVICE_CALLABLE_CLASS_MEMBER_HPP
#define SERVICE_CALLABLE_CLASS_MEMBER_HPP
#include "service/abstract_callable.hpp"
#include "serializer/counter.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/typed_byte_array_buffer.hpp"
#include "logger.hpp"

namespace fetch 
{
namespace service 
{
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

  /* A struct for invoking the member function once we have
   * unpacked all arguments.
   * @U is the return type.
   * @used_args are the types of the function arguments.
   *
   * This implementation invokes the member function with unpacked
   * arguments and packs the result using the supplied serializer.  
   */
  template< typename U, typename... used_args >
  struct Invoke 
  {
    /* Calls a member function with unpacked arguments.
     * @result is a serializer for storing the result.
     * @cls is the class instance.
     * @m is a pointer to the member function.
     * @used_args are the unpacked arguments.
     */
    static void MemberFunction(serializer_type &result, class_type &cls,
      member_function_pointer &m,
      used_args&... args) 
    {
      auto ret = (cls.*m)(args...);
      serializers::SizeCounter< serializer_type > counter;
      counter << ret;

      result.Reserve( counter.size() );
      result << ret;
    };
  };

  /* Special case for invocation with return type void.
   * @used_args are the types of the function arguments.
   *
   * In case of void as return type, the result is always 0 packed in a
   * uint8_t.
   */  
  template< typename... used_args >
  struct Invoke<void, used_args...> 
  {
    static void MemberFunction(serializer_type &result, class_type &cls,
      member_function_pointer &m,
      used_args&... args) 
    {
      result << uint8_t(0);
      (cls.*m)(args...);
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
      static void Unroll(serializer_type &result, class_type &cls,
        member_function_pointer &m, serializer_type &s,
        used_args &... used) 
      {
        typename std::decay<T>::type l; 
        s >> l;
        UnrollArguments<used_args..., T>::template LoopOver<
          remaining_args...>::Unroll(result, cls, m, s, used..., l);
      }
    };
    
    /* Struct for loop termination
     * @T is the type of the last argument to be unrolled.
     */  
    template <typename T>
    struct LoopOver<T> 
    {
      static void Unroll(serializer_type &result, class_type &cls,
        member_function_pointer &m, serializer_type &s,
        used_args &... used) 
      {
        typename std::decay<T>::type l;
        
        s >> l;
        Invoke<return_type, used_args..., T>::MemberFunction(result, cls, m, used..., l);
      }
    };
  };


  /* Struct used for unrolling arguments in a function signature.
   * @used_args are the unpacked arguments.
   */  
  template< std::size_t COUNTER>
  struct UnrollPointers
  {

    template <typename... used_args>
    struct WithUsed 
    {
      
    /* Struct for loop definition.
     * @T is the type of the next argument to be unrolled.
     * @remaining_args are the arugments which has not yet been unrolled.
     */  
    template <typename T, typename... remaining_args>
    struct LoopOver 
    {
      static void Unroll(serializer_type &result, class_type &cls,
        member_function_pointer &m, serializer_type &s,
        std::vector< void* > const &additional_args,
        used_args &... used) 
      {
        typename std::decay<T>::type* ptr = (typename std::decay<T>::type*)additional_args[COUNTER - 1]; 
        std::cout << "Unrolling element " << COUNTER - 1 << std::endl;
        
        UnrollPointers<used_args..., T>::template LoopOver<COUNTER - 1,
          remaining_args...>::Unroll(result, cls, m, s, additional_args, used..., *ptr);
      }
    };
    
    /* Struct for loop termination
     * @T is the type of the last argument to be unrolled.
     */  
    template <typename... remaining_args>
    struct LoopOver<remaining_args...> 
    {
      static void Unroll(serializer_type &result, class_type &cls,
        member_function_pointer &m, serializer_type &s,
        std::vector< void* > const &additional_args,
        used_args &... used) 
      {

      }
    };
      
    };    
  };

  template<>
  struct UnrollPointers<0> 
  {
    template <typename... used_args>
    struct WithUsed 
    {
      template <typename... remaining_args>
      struct LoopOver 
      {
        std::cout << "TODO: unroll serialization args" << std::endl;        
      };
    };
  };
  
    
  
public:
  /* Creates a callable class member.
   * @cls is the class instance.
   * @function is the member function.
   */
  CallableClassMember(class_type *cls, member_function_pointer function) 
  {
    LOG_STACK_TRACE_POINT;
    
    class_ = cls;
    function_ = function;
  }


  CallableClassMember(uint64_t arguments, class_type *cls, member_function_pointer value) :
    AbstractCallable(arguments)
  {
    LOG_STACK_TRACE_POINT;
    
    class_ = cls;
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
    
    UnrollArguments<>::template LoopOver<Args...>::Unroll(
      result, *class_, this->function_, params);
  }


  void operator()(serializer_type &result, std::vector< void* > const &additional_args, serializer_type &params) 
  {
    LOG_STACK_TRACE_POINT;
    assert(N == additional_args.size());
    
    UnrollPointers<>::template LoopOver<N, Args...>::Unroll(
      result, *class_, this->function_, params);
  }
  
private:
  class_type *class_;
  member_function_pointer function_;
};


// No function args
template <typename C, typename R>
class CallableClassMember<C, R()> : public AbstractCallable 
{
private:
  typedef R return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)();

public:
  CallableClassMember(class_type *cls, member_function_pointer value) 
  {
    LOG_STACK_TRACE_POINT;
    
    class_ = cls;
    function_ = value;
  }

  CallableClassMember(uint64_t arguments, class_type *cls, member_function_pointer value) :
    AbstractCallable(arguments)
  {
    LOG_STACK_TRACE_POINT;
    
    class_ = cls;
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override 
  {
    LOG_STACK_TRACE_POINT;

    auto ret =  ( (*class_).*function_)();
    serializers::SizeCounter< serializer_type > counter;
    counter << ret;
    result.Reserve( counter.size() );
    result << ret;
  }

private:
  class_type *class_;
  member_function_pointer function_;
};

// No function args, void return
template <typename C>
class CallableClassMember<C, void()> : public AbstractCallable 
{
private:
  typedef void return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)();

public:    
  CallableClassMember(class_type *cls, member_function_pointer value) 
  {
    LOG_STACK_TRACE_POINT;
    
    class_ = cls;
    function_ = value;
  }

  CallableClassMember(uint64_t arguments, class_type *cls, member_function_pointer value) :
    AbstractCallable(arguments)
  {
    LOG_STACK_TRACE_POINT;
    
    class_ = cls;
    function_ = value;
  }
  
  
  void operator()(serializer_type &result, serializer_type &params) override 
  {
    LOG_STACK_TRACE_POINT;
    
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
