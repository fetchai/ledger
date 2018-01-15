#ifndef RPC_CALLABLE_CLASS_MEMBER_HPP
#define RPC_CALLABLE_CLASS_MEMBER_HPP
#include "rpc/abstract_callable.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/typed_byte_array_buffer.hpp"

namespace fetch {
namespace rpc {

template <typename C, typename F>
class CallableClassMember;

template <typename C, typename R, typename... Args>
class CallableClassMember<C, R(Args...)> : public AbstractCallable {
 private:
  typedef R return_type;
  typedef C class_type;
  typedef return_type (class_type::*member_function_pointer)(Args...);

  template <typename... used_args>
  static void InvokeMemberFunction(serializer_type &result, class_type &cls,
                                   member_function_pointer &m,
                                   used_args... args) {
    result << (cls.*m)(args...);
  };

  template <typename... used_args>
  struct UnrollArguments {
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

    template <typename T>
    struct LoopOver<T> {
      static void Unroll(serializer_type &result, class_type &cls,
                         member_function_pointer &m, serializer_type &s,
                         used_args &... used) {
        T l;
        s >> l;
        InvokeMemberFunction(result, cls, m, used..., l);
      }
    };
  };

 public:
  CallableClassMember(class_type *cls, member_function_pointer value) {
    class_ = cls;
    function_ = value;
  }

  void operator()(serializer_type &result, serializer_type &params) override {
    UnrollArguments<>::template LoopOver<Args...>::Unroll(
        result, *class_, this->function_, params);
  }

 private:
  class_type *class_;
  member_function_pointer function_;
};
};
};

#endif
