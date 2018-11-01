#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/meta/type_traits.hpp"
#include "core/random.hpp"
#include "math/kernels/standard_deviation.hpp"
#include "math/kernels/standard_functions.hpp"
#include "math/kernels/variance.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/range.hpp"
#include "vectorise/memory/shared_array.hpp"

#include "math/free_functions/free_functions.hpp"
#include "math/statistics/mean.hpp"

#include <algorithm>
#include <type_traits>
#include <vector>

namespace fetch {
namespace math {

namespace details {
template <typename DataType, typename ArrayType>
static void ArangeImplementation(DataType from, DataType to, DataType delta, ArrayType &ret)
{
  std::size_t N = std::size_t((to - from) / delta);
  ret.LazyResize(N);
  ret.SetPaddedZero();
  ret.FillArange(from, to);
}
}  // namespace details

template <typename T, typename C = memory::SharedArray<T>>
class ShapeLessArray
{
public:
  using Type                          = T;
  using container_type                = C;
  using size_type                     = std::size_t;
  using vector_slice_type             = typename container_type::vector_slice_type;
  using vector_register_type          = typename container_type::vector_register_type;
  using vector_register_iterator_type = typename container_type::vector_register_iterator_type;
  using self_type                     = ShapeLessArray<T, C>;

  /* Iterators for accessing and modifying the array */
  using iterator         = typename container_type::iterator;
  using reverse_iterator = typename container_type::reverse_iterator;

  // TODO(private issue 282): This probably needs to be removed into the meta
  template <typename Type, typename ReturnType = void>
  using IsUnsignedLike =
      typename std::enable_if<std::is_integral<Type>::value && std::is_unsigned<Type>::value,
                              ReturnType>::type;
  template <typename Type, typename ReturnType = void>
  using IsSignedLike =
      typename std::enable_if<std::is_integral<Type>::value && std::is_signed<Type>::value,
                              ReturnType>::type;
  template <typename Type, typename ReturnType = void>
  using IsIntegralLike = typename std::enable_if<std::is_integral<Type>::value, ReturnType>::type;

  static constexpr char const *LOGGING_NAME = "ShapeLessArray";

  /* Contructs an empty shape-less array. */
  ShapeLessArray(std::size_t const &n)
    : data_(n)
    , size_(n)
  {}

  ShapeLessArray()
    : data_()
    , size_(0)
  {}

  ShapeLessArray(ShapeLessArray &&other)      = default;
  ShapeLessArray(ShapeLessArray const &other) = default;
  ShapeLessArray &operator=(ShapeLessArray const &other) = default;
  ShapeLessArray &operator=(ShapeLessArray &&other) = default;
  ShapeLessArray(byte_array::ConstByteArray const &c)
    : data_()
    , size_(0)
  {
    // TODO(private issue 226): Make this a static function and add failure mechanism
    std::vector<Type> elems;
    elems.reserve(1024);

    for (uint64_t i = 0; i < c.size();)
    {
      uint64_t last = i;
      switch (c[i])
      {
      case ',':
      case ' ':
      case '\n':
      case '\t':
      case '\r':
        ++i;
        break;
      default:
        if (byte_array::consumers::NumberConsumer<1, 2>(c, i) == -1)
        {
          // TODO(private issue 226): failed
        }
        else
        {
          // FIXME(tfr) : This is potentially wrong! Error also there in matrix
          // problem is that the pointer is not necessarily null-terminated.
          elems.push_back(Type(atof(c.char_pointer() + last)));
        }
        break;
      }
    }

    std::size_t m = elems.size();
    this->Resize(m);
    this->SetAllZero();

    for (std::size_t i = 0; i < m; ++i)
    {
      this->Set(i, elems[i]);
    }
  }

  ~ShapeLessArray()
  {}

  /* Set all elements to zero.
   *
   * This method will initialise all memory with zero.
   */
  void SetAllZero()
  {
    data().SetAllZero();
  }

  /**
   * Inefficient implementation of set all one. A low level method in memory::SharedArray would be
   * preferable
   */
  void SetAllOne()
  {
    for (std::size_t i = 0; i < data().size(); i++)
    {
      data()[i] = 1;
    }
  }

  /* Set all padded bytes to zero.
   *
   * This method sets the padded bytes to zero. Padded bytes are those
   * which are added to ensure that the arrays true size is a multiple
   * of the vector unit.
   */
  void SetPaddedZero()
  {
    data().SetPaddedZero();
  }

  void Sort()
  {
    std::sort(data_.pointer(), data_.pointer() + data_.size());
  }

  void Sort(memory::TrivialRange const &range)
  {
    std::sort(data_.pointer() + range.from(), data_.pointer() + range.to());
  }

  void Fill(Type const &value, memory::Range const &range)
  {

    if (range.is_undefined())
    {
      Fill(value);
    }
    else if (range.is_trivial())
    {
      Fill(value, range.ToTrivialRange(this->size()));
    }
    else
    {
      TODO_FAIL("Support for general range is not implmenented yet");
    }
  }

  void Fill(Type const &value, memory::TrivialRange const &range)
  {
    vector_register_type val(value);

    this->data().in_parallel().Apply(range, [val](vector_register_type &z) { z = val; });
  }

  void Fill(Type const &value)
  {
    vector_register_type val(value);

    this->data().in_parallel().Apply([val](vector_register_type &z) { z = val; });
  }

  void Equal(self_type const &a, self_type const &b)
  {
    assert(a.size() == b.size());
    this->Resize(a.size());

    this->data_.in_parallel().Apply([](vector_register_type const &a, vector_register_type const &b,
                                       vector_register_type &c) { c = (a == b); },
                                    a.data(), b.data());
  }

  void NotEqual(self_type const &a, self_type const &b)
  {
    assert(a.size() == b.size());
    this->Resize(a.size());

    this->data_.in_parallel().Apply([](vector_register_type const &a, vector_register_type const &b,
                                       vector_register_type &c) { c = (a != b); },
                                    a.data(), b.data());
  }

  void LessThan(self_type const &a, self_type const &b)
  {
    assert(a.size() == b.size());
    this->Resize(a.size());

    this->data_.in_parallel().Apply([](vector_register_type const &a, vector_register_type const &b,
                                       vector_register_type &c) { c = (a < b); },
                                    a.data(), b.data());
  }

  void LessThanEqual(self_type const &a, self_type const &b)
  {
    assert(a.size() == b.size());
    this->Resize(a.size());

    this->data_.in_parallel().Apply([](vector_register_type const &a, vector_register_type const &b,
                                       vector_register_type &c) { c = (a <= b); },
                                    a.data(), b.data());
  }

  void GreaterThan(self_type const &a, self_type const &b)
  {
    assert(a.size() == b.size());
    this->Resize(a.size());

    this->data_.in_parallel().Apply([](vector_register_type const &a, vector_register_type const &b,
                                       vector_register_type &c) { c = (a > b); },
                                    a.data(), b.data());
  }

  void GreaterThanEqual(self_type const &a, self_type const &b)
  {
    assert(a.size() == b.size());
    this->Resize(a.size());

    this->data_.in_parallel().Apply([](vector_register_type const &a, vector_register_type const &b,
                                       vector_register_type &c) { c = (a >= b); },
                                    a.data(), b.data());
  }

  /*
    void Exp(self_type const &x) {
      LazyResize( x.size() );

      kernels::ApproxExp< vector_register_type > aexp;
      data_.in_parallel().Apply(aexp, x.data_);
    }
  */

  void ApproxSoftMax(self_type const &x)
  {
    //    kernels::ApproxSoftMax< Type, vector_register_type > kernel;
    //    kernel( this->data_, x.data());
  }

  /**
   * calculates the l2loss of data in the array
   *
   * @return       returns single value as Type
   *
   **/
  Type L2Loss() const
  {
    Type sum = data_.in_parallel().SumReduce([](vector_register_type const &v) { return v * v; });
    return sum * Type(0.5);
  }

  /**
   * Divide this array by another shapeless array and store the floating point remainder in this
   * array
   * @param x
   */
  void Fmod(self_type const &x)
  {
    LazyResize(x.size());
    fetch::math::Fmod(data_, x.data(), data_);
  }

  /**
   * Divide this array by another shapeless array and store the remainder in this array with
   * quotient rounded to int
   * @param x
   */
  void Remainder(self_type const &x)
  {
    LazyResize(x.size());
    fetch::math::Remainder(data_, x.data(), data_);
  }

  void Remquo(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Remquo<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fma(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fma<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fmax(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fmax<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fmin(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fmin<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fdim(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fdim<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nan(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nan<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nanf(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nanf<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nanl(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nanl<Type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  /**
   * Apply softmax to this array
   * @param x
   * @return
   */
  self_type Softmax(self_type const &x)
  {
    LazyResize(x.size());

    assert(x.size() == this->size());
    fetch::math::Softmax(x, *this);

    return *this;
  }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   *
   * Note this accessor is "slow" as it takes care that the developer
   * does not accidently enter the padded area of the memory.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type const &At(S const &i) const
  {
    return data_[i];
  }

  /* One-dimensional reference access function.
   * @param i is the index which is being accessed.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &At(S const &i)
  {
    return data_[i];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type const &Set(S const &   i,
                                                                             Type const &t)
  {
    return data_[i] = t;
  }

  template <typename Unsigned>
  static IsUnsignedLike<Unsigned, ShapeLessArray> Arange(Unsigned from, Unsigned to, Unsigned delta)
  {
    ShapeLessArray ret;

    std::size_t N = std::size_t((to - from) / delta);
    ret.LazyResize(N);
    ret.SetPaddedZero();
    ret.FillArange(from, to);

    return ret;
  }

  template <typename Unsigned>
  IsUnsignedLike<Unsigned, ShapeLessArray &> FillArange(Unsigned from, Unsigned const &to)
  {
    assert(from < to);

    std::size_t N     = this->size();
    Type        d     = static_cast<Type>(from);
    Type        delta = static_cast<Type>(to - from) / static_cast<Type>(N);

    for (std::size_t i = 0; i < N; ++i)
    {
      this->data()[i] = Type(d);
      d += delta;
    }
    return *this;
  }

  static ShapeLessArray UniformRandom(std::size_t const &N)
  {

    ShapeLessArray ret;
    ret.LazyResize(N);
    ret.SetPaddedZero();
    ret.FillUniformRandom();

    return ret;
  }

  static ShapeLessArray UniformRandomIntegers(std::size_t const &N, int64_t const &min,
                                              int64_t const &max)
  {
    ShapeLessArray ret;
    ret.LazyResize(N);
    ret.SetPaddedZero();
    ret.FillUniformRandomIntegers(min, max);

    return ret;
  }

  ShapeLessArray &FillUniformRandom()
  {
    for (std::size_t i = 0; i < this->size(); ++i)
    {
      this->data()[i] = Type(random::Random::generator.AsDouble());
    }
    return *this;
  }

  ShapeLessArray &FillUniformRandomIntegers(int64_t const &min, int64_t const &max)
  {
    assert(min <= max);

    uint64_t diff = uint64_t(max - min);

    for (std::size_t i = 0; i < this->size(); ++i)
    {
      this->data()[i] = Type(int64_t(random::Random::generator() % diff) + min);
    }

    return *this;
  }

  static ShapeLessArray Zeroes(std::size_t const &n)
  {
    ShapeLessArray ret;
    ret.Resize(n);
    ret.SetAllZero();
    return ret;
  }

  /**
   * Method returning a shapeless array of ones
   *
   * @param shape : a vector representing the shape of the NDArray
   * @return NDArray with all ones
   */
  static ShapeLessArray Ones(std::size_t const &n)
  {
    ShapeLessArray ret;
    ret.Resize(n);
    ret.SetAllOne();
    return ret;
  }

  bool AllClose(ShapeLessArray const &other, double const &rtol = 1e-5, double const &atol = 1e-8,
                bool ignoreNaN = true) const
  {
    std::size_t N = this->size();
    if (other.size() != N)
    {
      return false;
    }
    bool ret = true;
    for (std::size_t i = 0; ret && i < N; ++i)
    {
      double va = this->At(i);
      if (ignoreNaN && std::isnan(va))
      {
        continue;
      }
      double vb = other[i];
      if (ignoreNaN && std::isnan(vb))
      {
        continue;
      }
      double vA = (va - vb);
      if (vA < 0)
      {
        vA = -vA;
      }
      if (va < 0)
      {
        va = -va;
      }
      if (vb < 0)
      {
        vb = -vb;
      }
      double M = std::max(va, vb);

      ret &= (vA < std::max(atol, M * rtol));
    }
    if (!ret)
    {
      for (std::size_t i = 0; i < N; ++i)
      {
        double va = this->At(i);
        if (ignoreNaN && std::isnan(va))
        {
          continue;
        }
        double vb = other[i];
        if (ignoreNaN && std::isnan(vb))
        {
          continue;
        }
        double vA = (va - vb);
        if (vA < 0)
        {
          vA = -vA;
        }
        if (va < 0)
        {
          va = -va;
        }
        if (vb < 0)
        {
          vb = -vb;
        }
        double M = std::max(va, vb);
        std::cout << this->At(i) << " " << other[i] << " "
                  << ((vA < std::max(atol, M * rtol)) ? " " : "*") << std::endl;
      }
    }

    return ret;
  }

  bool LazyReserve(std::size_t const &n)
  {
    if (data_.size() < n)
    {
      data_ = container_type(n);
      return true;
    }
    return false;
  }

  void Reserve(std::size_t const &n)
  {
    container_type old_data = data_;

    if (LazyReserve(n))
    {
      std::size_t ns = std::min(old_data.size(), n);
      memcpy(data_.pointer(), old_data.pointer(), ns);
      data_.SetZeroAfter(ns);
    }
  }

  void ReplaceData(std::size_t const &n, container_type const &data)
  {
    assert(n <= data.size());
    data_ = data;
    size_ = n;
  }

  void LazyResize(std::size_t const &n)
  {
    LazyReserve(n);
    size_ = n;
    data_.SetZeroAfter(n);
  }

  void Resize(std::size_t const &n)
  {
    container_type old_data = data_;
    LazyResize(n);
    size_ = n;
  }

  iterator begin()
  {
    return data_.begin();
  }
  iterator end()
  {
    return data_.end();
  }
  reverse_iterator rbegin()
  {
    return data_.rbegin();
  }
  reverse_iterator rend()
  {
    return data_.rend();
  }

  // TODO(TFR): deduce D from parent
  template <typename S, typename D = memory::SharedArray<S>>
  void As(ShapeLessArray<S, D> &ret) const
  {
    ret.LazyResize(size_);
    // TODO(TFR): Vectorize
    for (std::size_t i = 0; i < size_; ++i)
    {
      ret.data_[i] = data_[i];
    }
  }

  self_type Copy() const
  {
    self_type copy;
    copy.data_ = this->data_.Copy();
    copy.size_ = this->size_;

    return copy;
  }

  void Copy(self_type const &x)
  {
    this->data_ = x.data_.Copy();
    this->size_ = x.size_;
  }

  void Set(std::size_t const &idx, Type const &val)
  {
    data_[idx] = val;
  }

  template <typename S>
  fetch::meta::IfIsUnsignedLike<S, Type> Get(S const &indices) const
  {
    return data_[indices];
  }
  //  T Get(std::size_t const &idx) { return data_[idx]; } const

  container_type const &data() const
  {
    return data_;
  }
  container_type &data()
  {
    return data_;
  }
  std::size_t size() const
  {
    return size_;
  }

  /* Returns the capacity of the array. */
  size_type capacity() const
  {
    return data_.padded_size();
  }
  size_type padded_size() const
  {
    return data_.padded_size();
  }

  ShapeLessArray &InlineAdd(ShapeLessArray const &other, memory::Range const &range)
  {
    assert(other.size() == this->size());

    if (range.is_undefined())
    {
      InlineAdd(other);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());
      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x + y; },
          this->data(), other.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &InlineAdd(ShapeLessArray const &other)
  {
    memory::Range range{0, other.data().size(), 1};
    return InlineAdd(other, range);
  }

  ShapeLessArray &InlineAdd(Type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x + val; },
        this->data());

    return *this;
  }

  ShapeLessArray &InlineMultiply(ShapeLessArray const &other, memory::Range const &range)
  {
    assert(other.size() == this->size());
    if (range.is_undefined())
    {
      InlineMultiply(other);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());
      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x * y; },
          this->data(), other.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &InlineMultiply(ShapeLessArray const &other)
  {
    memory::Range range{0, other.data().size(), 1};
    return InlineMultiply(other, range);
  }

  ShapeLessArray &InlineMultiply(Type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x * val; },
        this->data());

    return *this;
  }

  ShapeLessArray &InlineSubtract(ShapeLessArray const &other, memory::Range const &range)
  {
    assert(other.size() == this->size());

    if (range.is_undefined())
    {
      InlineSubtract(other);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());
      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x - y; },
          this->data(), other.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &InlineSubtract(ShapeLessArray const &other)
  {
    memory::Range range{0, other.data().size(), 1};
    return InlineSubtract(other, range);
  }

  ShapeLessArray &InlineReverseSubtract(ShapeLessArray const &other, memory::Range const &range)
  {
    assert(other.size() == this->size());

    if (range.is_undefined())
    {
      InlineSubtract(other);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());
      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = y - x; },
          this->data(), other.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &InlineReverseSubtract(ShapeLessArray const &other)
  {
    memory::Range range{0, other.data().size(), 1};
    return InlineReverseSubtract(other, range);
  }

  ShapeLessArray &InlineSubtract(Type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &y, vector_register_type &z) { z = y - val; },
        this->data());

    return *this;
  }

  ShapeLessArray &InlineDivide(ShapeLessArray const &other, memory::Range const &range)
  {
    assert(other.size() == this->size());

    if (range.is_undefined())
    {
      InlineDivide(other);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());
      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x / y; },
          this->data(), other.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &InlineDivide(ShapeLessArray const &other)
  {
    memory::Range range{0, other.data().size(), 1};
    return InlineDivide(other, range);
  }

  ShapeLessArray &InlineDivide(Type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &y, vector_register_type &z) { z = y / val; },
        this->data());

    return *this;
  }

  ShapeLessArray &InlineReverseSubtract(Type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &y, vector_register_type &z) { z = val - y; },
        this->data());

    return *this;
  }

  ShapeLessArray &InlineReverseDivide(ShapeLessArray const &other, memory::Range const &range)
  {
    assert(other.size() == this->size());

    if (range.is_undefined())
    {
      InlineDivide(other);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());
      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = y / x; },
          this->data(), other.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &InlineReverseDivide(ShapeLessArray const &other)
  {
    memory::Range range{0, other.data().size(), 1};
    return InlineReverseDivide(other, range);
  }

  ShapeLessArray &InlineReverseDivide(Type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &y, vector_register_type &z) { z = val / y; },
        this->data());

    return *this;
  }

  /////////////////
  /// OPERATORS ///
  /////////////////

  /**
   * Equality operator
   * This method is sensitive to height and width
   * @param other  the array which this instance is compared against
   * @return
   */
  bool operator==(ShapeLessArray const &other) const
  {
    if (size() != other.size())
    {
      return false;
    }
    bool ret = true;

    for (size_type i = 0; ret && i < data().size(); ++i)
    {
      ret &= (data()[i] == other.data()[i]);
    }

    return ret;
  }

  /**
   * Not-equal operator
   * This method is sensitive to height and width
   * @param other the array which this instance is compared against
   * @return
   */
  bool operator!=(ShapeLessArray const &other) const
  {
    return !(this->operator==(other));
  }

  /**
   * + operator
   * @tparam OtherType may be a scalar or array, but must be arithmetic
   * @param other
   * @return
   */
  template <typename OtherType>
  ShapeLessArray operator+(OtherType const &other)
  {
    fetch::math::Add(*this, other, *this);
    return *this;
  }

  /* One-dimensional reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that is
   * meant for non-constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type &operator[](S const &i)
  {
    return data_[i];
  }

  /* One-dimensional constant reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that can be
   * used for constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, Type>::type const &operator[](
      S const &i) const
  {
    return data_[i];
  }

  ///////////////////////////////////////
  /// MATH LIBRARY INTERFACE METHODS ////
  ///////////////////////////////////////

  Type PeakToPeak() const
  {
    return fetch::math::PeakToPeak(*this);
  }

protected:
  container_type data_;
  std::size_t    size_ = 0;
};
}  // namespace math
}  // namespace fetch
