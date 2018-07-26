#pragma once
#include "vectorise/info.hpp"
#include "vectorise/info_sse.hpp"
#include "vectorise/register.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

namespace fetch {
namespace vectorize {

namespace details {
template <typename T, std::size_t N>
struct UnrollSet
{
  static void Set(T *ptr, T const &c)
  {
    (*ptr) = c;
    UnrollSet<T, N - 1>::Set(ptr + 1, c);
  }
};

template <typename T>
struct UnrollSet<T, 0>
{
  static void Set(T *ptr, T const &c) {}
};
}  // namespace details

// SSE integers
template <typename T>
class VectorRegister<T, 128>
{
public:
  using type             = T;
  using mm_register_type = __m128i;

  enum
  {
    E_VECTOR_SIZE   = 128,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) { data_ = _mm_load_si128((mm_register_type *)d); }
  VectorRegister(type const &c)
  {
    alignas(16) type constant[E_BLOCK_COUNT];
    details::UnrollSet<type, E_BLOCK_COUNT>::Set(constant, c);
    data_ = _mm_load_si128((mm_register_type *)constant);
  }
  VectorRegister(mm_register_type const &d) : data_(d) {}
  VectorRegister(mm_register_type &&d) : data_(d) {}

  explicit operator mm_register_type() { return data_; }

  void Store(type *ptr) const { _mm_store_si128((mm_register_type *)ptr, data_); }
  void Stream(type *ptr) const { _mm_stream_si128((mm_register_type *)ptr, data_); }

  mm_register_type const &data() const { return data_; }
  mm_register_type &      data() { return data_; }

private:
  mm_register_type data_;
};

template <>
class VectorRegister<float, 128>
{
public:
  using type             = float;
  using mm_register_type = __m128;

  enum
  {
    E_VECTOR_SIZE   = 128,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) { data_ = _mm_load_ps(d); }
  VectorRegister(mm_register_type const &d) : data_(d) {}
  VectorRegister(mm_register_type &&d) : data_(d) {}
  VectorRegister(type const &c) { data_ = _mm_load_ps1(&c); }

  explicit operator mm_register_type() { return data_; }

  void Store(type *ptr) const { _mm_store_ps(ptr, data_); }
  void Stream(type *ptr) const { _mm_stream_ps(ptr, data_); }

  mm_register_type const &data() const { return data_; }
  mm_register_type &      data() { return data_; }

private:
  mm_register_type data_;
};

template <>
class VectorRegister<double, 128>
{
public:
  using type             = double;
  using mm_register_type = __m128d;

  enum
  {
    E_VECTOR_SIZE   = 128,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() {}
  VectorRegister(type const *d) { data_ = _mm_load_pd(d); }
  VectorRegister(mm_register_type const &d) : data_(d) {}
  VectorRegister(mm_register_type &&d) : data_(d) {}
  VectorRegister(type const &c) { data_ = _mm_load_pd1(&c); }

  explicit operator mm_register_type() { return data_; }

  void Store(type *ptr) const { _mm_stream_pd(ptr, data_); }
  void Stream(type *ptr) const { _mm_stream_pd(ptr, data_); }

  mm_register_type const &data() const { return data_; }
  mm_register_type &      data() { return data_; }

private:
  mm_register_type data_;
};

#define FETCH_ADD_OPERATOR(zero, type, fnc)                                      \
  inline VectorRegister<type, 128> operator-(VectorRegister<type, 128> const &x) \
  {                                                                              \
    return VectorRegister<type, 128>(fnc(zero(), x.data()));                     \
  }

FETCH_ADD_OPERATOR(_mm_setzero_si128, int, _mm_sub_epi32)
FETCH_ADD_OPERATOR(_mm_setzero_ps, float, _mm_sub_ps)
FETCH_ADD_OPERATOR(_mm_setzero_pd, double, _mm_sub_pd)
#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 128> operator op(VectorRegister<type, 128> const &a, \
                                               VectorRegister<type, 128> const &b) \
  {                                                                                \
    L ret = fnc(a.data(), b.data());                                               \
    return VectorRegister<type, 128>(ret);                                         \
  }

FETCH_ADD_OPERATOR(*, int, __m128i, _mm_mullo_epi32)
FETCH_ADD_OPERATOR(-, int, __m128i, _mm_sub_epi32)
// FETCH_ADD_OPERATOR(/, int, __m128i, _mm_div_epi32);
FETCH_ADD_OPERATOR(+, int, __m128i, _mm_add_epi32)

FETCH_ADD_OPERATOR(==, int, __m128i, _mm_cmpeq_epi32)
// FETCH_ADD_OPERATOR(!=, int, __m128i, _mm_cmpneq_epi32)
// FETCH_ADD_OPERATOR(>=, int, __m128i, _mm_cmpge_epi32)
// FETCH_ADD_OPERATOR(>, int, __m128i, _mm_cmpgt_epi32)
// FETCH_ADD_OPERATOR(<=, int, __m128i, _mm_cmple_epi32)
FETCH_ADD_OPERATOR(<, int, __m128i, _mm_cmplt_epi32)

FETCH_ADD_OPERATOR(*, float, __m128, _mm_mul_ps)
FETCH_ADD_OPERATOR(-, float, __m128, _mm_sub_ps)
FETCH_ADD_OPERATOR(/, float, __m128, _mm_div_ps)
FETCH_ADD_OPERATOR(+, float, __m128, _mm_add_ps)

FETCH_ADD_OPERATOR(*, double, __m128d, _mm_mul_pd)
FETCH_ADD_OPERATOR(-, double, __m128d, _mm_sub_pd)
FETCH_ADD_OPERATOR(/, double, __m128d, _mm_div_pd)
FETCH_ADD_OPERATOR(+, double, __m128d, _mm_add_pd)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 128> operator op(VectorRegister<type, 128> const &a, \
                                               VectorRegister<type, 128> const &b) \
  {                                                                                \
    L              imm  = fnc(a.data(), b.data());                                 \
    __m128i        ival = _mm_castps_si128(imm);                                   \
    constexpr type done = type(1);                                                 \
    const __m128i  one  = _mm_castps_si128(_mm_load_ps1(&done));                   \
    __m128i        ret  = _mm_and_si128(ival, one);                                \
    return VectorRegister<type, 128>(_mm_castsi128_ps(ret));                       \
  }

FETCH_ADD_OPERATOR(==, float, __m128, _mm_cmpeq_ps)
FETCH_ADD_OPERATOR(!=, float, __m128, _mm_cmpneq_ps)
FETCH_ADD_OPERATOR(>=, float, __m128, _mm_cmpge_ps)
FETCH_ADD_OPERATOR(>, float, __m128, _mm_cmpgt_ps)
FETCH_ADD_OPERATOR(<=, float, __m128, _mm_cmple_ps)
FETCH_ADD_OPERATOR(<, float, __m128, _mm_cmplt_ps)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(op, type, L, fnc)                                       \
  inline VectorRegister<type, 128> operator op(VectorRegister<type, 128> const &a, \
                                               VectorRegister<type, 128> const &b) \
  {                                                                                \
    L              imm  = fnc(a.data(), b.data());                                 \
    __m128i        ival = _mm_castpd_si128(imm);                                   \
    constexpr type done = type(1);                                                 \
    const __m128i  one  = _mm_castpd_si128(_mm_load_pd1(&done));                   \
    __m128i        ret  = _mm_and_si128(ival, one);                                \
    return VectorRegister<type, 128>(_mm_castsi128_pd(ret));                       \
  }

FETCH_ADD_OPERATOR(==, double, __m128d, _mm_cmpeq_pd)
FETCH_ADD_OPERATOR(!=, double, __m128d, _mm_cmpneq_pd)
FETCH_ADD_OPERATOR(>=, double, __m128d, _mm_cmpge_pd)
FETCH_ADD_OPERATOR(>, double, __m128d, _mm_cmpgt_pd)
FETCH_ADD_OPERATOR(<=, double, __m128d, _mm_cmple_pd)
FETCH_ADD_OPERATOR(<, double, __m128d, _mm_cmplt_pd)

// Manage NaN
//__m128d _mm_cmpord_pd (__m128d a, __m128d b)
//__m128d _mm_cmpunord_pd (__m128d a, __m128d b)

#undef FETCH_ADD_OPERATOR

// FREE FUNCTIONS

inline VectorRegister<double, 128> vector_zero_below_element(VectorRegister<double, 128> const &a,
                                                             int const &                        n)
{
  alignas(16) uint64_t mask[2] = {uint64_t(-(0 >= n)), uint64_t(-(1 >= n))};

  __m128i conv = _mm_castpd_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i *>(mask));

  return VectorRegister<double, 128>(_mm_castsi128_pd(conv));
}

inline VectorRegister<double, 128> vector_zero_above_element(VectorRegister<double, 128> const &a,
                                                             int const &                        n)
{
  alignas(16) uint64_t mask[2] = {uint64_t(-(0 <= n)), uint64_t(-(1 <= n))};

  __m128i conv = _mm_castpd_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i *>(mask));

  return VectorRegister<double, 128>(_mm_castsi128_pd(conv));
}

inline VectorRegister<double, 128> shift_elements_left(VectorRegister<double, 128> const &x)
{
  __m128i n = _mm_castpd_si128(x.data());
  n         = _mm_bslli_si128(n, 8);
  return VectorRegister<double, 128>(_mm_castsi128_pd(n));
}

inline VectorRegister<double, 128> shift_elements_right(VectorRegister<double, 128> const &x)
{
  __m128i n = _mm_castpd_si128(x.data());
  n         = _mm_bsrli_si128(n, 8);
  return VectorRegister<double, 128>(_mm_castsi128_pd(n));
}

inline double first_element(VectorRegister<double, 128> const &x)
{
  return _mm_cvtsd_f64(x.data());
}

// Floats
inline VectorRegister<float, 128> vector_zero_below_element(VectorRegister<float, 128> const &a,
                                                            int const &                       n)
{
  alignas(16) const uint32_t mask[4] = {uint32_t(-(0 >= n)), uint32_t(-(1 >= n)),
                                        uint32_t(-(2 >= n)), uint32_t(-(3 >= n))};

  __m128i conv = _mm_castps_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i const *>(mask));

  return VectorRegister<float, 128>(_mm_castsi128_ps(conv));
}

inline VectorRegister<float, 128> vector_zero_above_element(VectorRegister<float, 128> const &a,
                                                            int const &                       n)
{
  alignas(16) const uint32_t mask[4] = {uint32_t(-(0 <= n)), uint32_t(-(1 <= n)),
                                        uint32_t(-(2 <= n)), uint32_t(-(3 <= n))};

  __m128i conv = _mm_castps_si128(a.data());
  conv         = _mm_and_si128(conv, *reinterpret_cast<__m128i const *>(mask));

  return VectorRegister<float, 128>(_mm_castsi128_ps(conv));
}

inline VectorRegister<float, 128> shift_elements_left(VectorRegister<float, 128> const &x)
{
  __m128i n = _mm_castps_si128(x.data());
  n         = _mm_bslli_si128(n, 4);
  return VectorRegister<float, 128>(_mm_castsi128_ps(n));
}

inline VectorRegister<float, 128> shift_elements_right(VectorRegister<float, 128> const &x)
{
  __m128i n = _mm_castps_si128(x.data());
  n         = _mm_bsrli_si128(n, 4);
  return VectorRegister<float, 128>(_mm_castsi128_ps(n));
}

inline float first_element(VectorRegister<float, 128> const &x) { return _mm_cvtss_f32(x.data()); }

// TODO(unknown): Rename and move
inline double reduce(VectorRegister<double, 128> const &x)
{
  __m128d r = _mm_hadd_pd(x.data(), _mm_setzero_pd());
  return _mm_cvtsd_f64(r);
}

inline float reduce(VectorRegister<float, 128> const &x)
{
  __m128 r = _mm_hadd_ps(x.data(), _mm_setzero_ps());
  r        = _mm_hadd_ps(r, _mm_setzero_ps());
  return _mm_cvtss_f32(r);
}

inline bool all_less_than(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128(_mm_cmplt_pd(x.data(), y.data()));
  return _mm_movemask_epi8(r) == 0xFFFF;
}

inline bool any_less_than(VectorRegister<double, 128> const &x,
                          VectorRegister<double, 128> const &y)
{
  __m128i r = _mm_castpd_si128(_mm_cmplt_pd(x.data(), y.data()));
  return _mm_movemask_epi8(r) != 0;
}

}  // namespace vectorize
}  // namespace fetch
