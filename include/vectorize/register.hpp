#ifndef VECTORIZE_REGISTER_HPP
#define VECTORIZE_REGISTER_HPP
#include "vectorize/vectorize_constants.hpp"

#include <emmintrin.h>

#include <smmintrin.h>
#include <type_traits>
#include <typeinfo>
#include<iostream>
namespace fetch {
namespace vectorize {

#define APPLY_OPERATOR_LIST(FUNCTION) \
  FUNCTION(*)                         \
  FUNCTION(/)                         \
  FUNCTION(+)                         \
  FUNCTION(-)                         \
  FUNCTION(&)                         \
  FUNCTION(|)                         \
  FUNCTION (^)

template <typename T, std::size_t L,
          InstructionSet I = InstructionSet::NO_VECTOR>
class VectorRegister {
 public:
  typedef T type;
  enum { E_REGISTER_SIZE = L, E_BLOCK_COUNT = L / sizeof(type) };
  static_assert((E_BLOCK_COUNT * sizeof(type)) == L,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) : data_(*d) {}
  VectorRegister(type const &d) : data_(d) {}
  VectorRegister(type &&d) : data_(d) {}

  explicit operator T() { return data_; }

#define AILIB_ADD_OPERATOR(OP)                              \
  VectorRegister operator OP(VectorRegister const &other) { \
    return VectorRegister(data_ OP other.data_);            \
  }
  APPLY_OPERATOR_LIST(AILIB_ADD_OPERATOR)
#undef AILIB_ADD_OPERATOR

  void Store(type *ptr) const { *ptr = data_; }

 private:
  type data_;
};

template <typename T, std::size_t L,
          InstructionSet I = InstructionSet::NO_VECTOR>
class VectorMemoryProxy {
 public:
  typedef VectorRegister<T, L, I> register_type;
  typedef T type;
  VectorMemoryProxy(type *ptr) : pointer_(ptr) {}

  explicit operator register_type() const { return register_type(pointer_); }

  template <typename G>
  explicit operator G() const {
    return G(register_type(pointer_));
  }

#define AILIB_ADD_OPERATOR(OP)                                           \
  register_type operator OP(VectorMemoryProxy const &other) {            \
    std::cout << "Proxy? " << typeid(register_type).name() << std::endl; \
    return register_type(*this) OP register_type(other);                 \
  }
  APPLY_OPERATOR_LIST(AILIB_ADD_OPERATOR)
#undef AILIB_ADD_OPERATOR

  register_type operator=(register_type const &reg) {
    reg.Store(pointer_);
    return reg;
  }

 private:
  type *pointer_;
};

template <typename T, InstructionSet I = InstructionSet::NO_VECTOR,
          std::size_t L = RegisterInfo<I, T>::size>
class VectorMemory {
 public:
  typedef VectorRegister<T, L, I> register_type;
  typedef VectorMemoryProxy<T, L, I> memory_proxy_type;
  typedef T type;
  VectorMemory() {}
  VectorMemory(type *ptr) : pointer_(ptr) {}

  //  VectorMemory operator+=() { return *this; }
  VectorMemory &operator++() {
    ++pointer_;
    return *this;
  }
  memory_proxy_type operator*() { return memory_proxy_type(pointer_); }
  memory_proxy_type operator[](std::size_t n) {
    return memory_proxy_type(pointer_ + n);
  }

 private:
  type *pointer_;
};
};
};
#endif
