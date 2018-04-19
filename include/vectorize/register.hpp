#ifndef VECTORIZE_REGISTER_HPP
#define VECTORIZE_REGISTER_HPP
#include "vectorize/info.hpp"

#include <type_traits>
#include <typeinfo>
#include <iostream>
#define APPLY_OPERATOR_LIST(FUNCTION) \
  FUNCTION(*)                         \
  FUNCTION(/)                         \
  FUNCTION(+)                         \
  FUNCTION(-)                         \
  FUNCTION(&)                         \
  FUNCTION(|)                         \
  FUNCTION (^)

namespace fetch {
namespace vectorize {

template <typename T, std::size_t N = sizeof(T), typename S = typename VectorInfo<T,N>::register_type  >
class VectorRegister {
 public:
  typedef T type;
  typedef S mm_register_type;
  
  enum {
    E_VECTOR_SIZE = sizeof(mm_register_type),        
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT = E_REGISTER_SIZE / sizeof(type)
  };
  
  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) : data_(*d) {}
  VectorRegister(type const &d) : data_(d) {}
  VectorRegister(type &&d) : data_(d) {}

  explicit operator T() { return data_; }

#define FETCH_ADD_OPERATOR(OP)                              \
  VectorRegister operator OP(VectorRegister const &other) { \
    return VectorRegister(data_ OP other.data_);            \
  }
  APPLY_OPERATOR_LIST( FETCH_ADD_OPERATOR );
#undef FETCH_ADD_OPERATOR

  void Store(type *ptr) const { *ptr = data_; }

  
 private:
  type data_;
};


#undef APPLY_OPERATOR_LIST
};
};
#endif
