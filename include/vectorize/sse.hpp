#ifndef VECTORIZE_SSE_HPPP
#define VECTORIZE_SSE_HPPP
#include "vectorize/vectorize_constants.hpp"
#include "vectorize/register.hpp"

#include <emmintrin.h>
#include <smmintrin.h>
#include <cstdint>
#include <cstddef>

#define REQUIRED_SSE X86_SSE3
namespace fetch {
namespace vectorize {
template <typename T, std::size_t L>
class VectorRegister<T, L, InstructionSet::REQUIRED_SSE> {
 public:
  typedef T type;
  enum { E_REGISTER_SIZE = L, E_BLOCK_COUNT = L / sizeof(type) };
  static_assert((E_BLOCK_COUNT * sizeof(type)) == L,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) { data_ = _mm_load_si128((__m128i *)d); }
  VectorRegister(__m128i const &d) : data_(d) {}
  VectorRegister(__m128i &&d) : data_(d) {}

  explicit operator __m128i() { return data_; }

#define AILIB_ADD_OPERATOR(OP) \
  VectorRegister operator OP(VectorRegister const &other) const;
  APPLY_OPERATOR_LIST(AILIB_ADD_OPERATOR)
#undef AILIB_ADD_OPERATOR

  void Store(type *ptr) const { _mm_store_si128((__m128i *)ptr, data_); }

 private:
  __m128i data_;
};

  /*
#define AILIB_ADD_OPERATOR(op, L, type, set, fnc)                            \
  template <>                                                                \
  VectorRegister<type, L, InstructionSet::set>                               \
      VectorRegister<type, L, InstructionSet::set>::operator op(             \
          VectorRegister<type, L, InstructionSet::set> const &other) const { \
    __m128i ret = fnc(this->data_, other.data_);                             \
    return VectorRegister<type, L, InstructionSet::set>(ret);                \
  }

AILIB_ADD_OPERATOR(*, 16, uint32_t, REQUIRED_SSE, _mm_mullo_epi32);
  
#undef AILIB_ADD_OPERATOR
  */
#undef REQUIRED_SSE
};
};
#endif
