#ifndef RPC_FUNCTION_HPP
#define RPC_FUNCTION_HPP
#include "rpc/abstract_callable.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/typed_byte_array_buffer.hpp"

#include<functional>

namespace fetch {
namespace rpc {

template <typename F>
class Function;

template <typename R, typename... Args>
class Function<R(Args...)> : public AbstractCallable {
 private:
  typedef R return_type;
  typedef std::function< R(Args...) > function_type;

   template< typename U, typename... used_args >
   struct Invoke {
     static void MemberFunction(serializer_type &result,
                                  function_type &m,
                                  used_args&... args) {
       result << return_type( m(args...) );
     };
       
   };
     
   template< typename... used_args >
   struct Invoke<void, used_args...> {
     static void MemberFunction(serializer_type &result,
                                function_type &m,
                                used_args&... args) {
       result << 0;
       m( args... );
     };
     
   };

  
  
  template <typename... used_args>
  struct UnrollArguments {
    template <typename T, typename... remaining_args>
    struct LoopOver {
      static void Unroll(serializer_type &result,
                         function_type &m, serializer_type &s,
                         used_args &... used) {
        T l;
        s >> l;
        UnrollArguments<used_args..., T>::template LoopOver<
            remaining_args...>::Unroll(result,  m, s, used..., l);
      }
    };

    template <typename T>
    struct LoopOver<T> {
      static void Unroll(serializer_type &result,
                         function_type &m, serializer_type &s,
                         used_args &... used) {
        T l;
        s >> l;
        Invoke<return_type, used_args..., T>::MemberFunction(result, m, used..., l);
      }
    };
  };

 public:
   
  Function(function_type value) {
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override {
    UnrollArguments<>::template LoopOver<Args...>::Unroll(
        result, this->function_, params);
  }

 private:

  function_type function_;
};


  // No function args
template <typename R>
class Function< R()> : public AbstractCallable {
 private:
  typedef R return_type;
  typedef std::function< R() > function_type;

 public:
  Function(function_type value) {
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override {
    result << R( function_() );
  }

 private:

  function_type function_;
};

  // No function args, void return
template <>
class Function<void()> : public AbstractCallable {
 private:
  typedef void return_type;
  typedef std::function< void() > function_type;


 public:
  Function(function_type value) {
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override {
    result << 0;
    function_();
  }

 private:
  function_type function_;
};
  
  
};
};

#endif
