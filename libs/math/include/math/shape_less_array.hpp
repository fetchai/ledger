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
#include "core/random.hpp"
#include "math/kernels/approx_exp.hpp"
#include "math/kernels/approx_log.hpp"
#include "math/kernels/approx_logistic.hpp"
#include "math/kernels/approx_soft_max.hpp"
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/relu.hpp"
#include "math/kernels/sign.hpp"
#include "math/kernels/standard_deviation.hpp"
#include "math/kernels/standard_functions.hpp"
#include "math/kernels/variance.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/range.hpp"
#include "vectorise/memory/shared_array.hpp"

#include "math/statistics/mean.hpp"

#include <algorithm>
#include <vector>

namespace fetch {
namespace math {

template <typename T, typename C = memory::SharedArray<T>>
class ShapeLessArray
{
public:
  using type                          = T;
  using container_type                = C;
  using size_type                     = std::size_t;
  using vector_slice_type             = typename container_type::vector_slice_type;
  using vector_register_type          = typename container_type::vector_register_type;
  using vector_register_iterator_type = typename container_type::vector_register_iterator_type;

  /* Iterators for accessing and modifying the array */
  using iterator         = typename container_type::iterator;
  using reverse_iterator = typename container_type::reverse_iterator;

  /* Contructs an empty shape-less array. */
  ShapeLessArray(std::size_t const &n) : data_(n), size_(n) {}

  ShapeLessArray() : data_(), size_(0) {}

  ShapeLessArray(ShapeLessArray &&other)      = default;
  ShapeLessArray(ShapeLessArray const &other) = default;
  ShapeLessArray &operator=(ShapeLessArray const &other) = default;
  ShapeLessArray &operator=(ShapeLessArray &&other) = default;
  ShapeLessArray(byte_array::ConstByteArray const &c) : data_(), size_(0)
  {
    std::vector<type> elems;
    elems.reserve(1024);
    bool failed = false;

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
          failed = true;
        }
        else
        {
          // FIXME(tfr) : This is potentially wrong! Error also there in matrix
          // problem is that the pointer is not necessarily null-terminated.
          elems.push_back(type(atof(c.char_pointer() + last)));
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

  ~ShapeLessArray() {}

  /* Set all elements to zero.
   *
   * This method will initialise all memory with zero.
   */
  void SetAllZero() { data().SetAllZero(); }

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
  void SetPaddedZero() { data().SetPaddedZero(); }

  using self_type = ShapeLessArray<T, C>;

  void Sort() { std::sort(data_.pointer(), data_.pointer() + data_.size()); }

  void Sort(memory::TrivialRange const &range)
  {
    std::sort(data_.pointer() + range.from(), data_.pointer() + range.to());
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

  ShapeLessArray &InlineAdd(type const &scalar)
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

  ShapeLessArray &InlineMultiply(type const &scalar)
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

  ShapeLessArray &InlineSubtract(type const &scalar)
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

  ShapeLessArray &InlineDivide(type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &y, vector_register_type &z) { z = y / val; },
        this->data());

    return *this;
  }

  ShapeLessArray &InlineReverseSubtract(type const &scalar)
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

  ShapeLessArray &InlineReverseDivide(type const &scalar)
  {
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &y, vector_register_type &z) { z = val / y; },
        this->data());

    return *this;
  }

  ShapeLessArray &Add(ShapeLessArray const &obj1, ShapeLessArray const &obj2,
                      memory::Range const &range)
  {
    assert(obj1.size() == obj2.size());
    assert(obj1.size() == this->size());

    if (range.is_undefined())
    {
      Add(obj1, obj2);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());

      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x + y; },
          obj1.data(), obj2.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &Add(ShapeLessArray const &obj1, ShapeLessArray const &obj2)
  {
    memory::Range range{0, std::min(obj1.data().size(), obj2.data().size()), 1};
    return Add(obj1, obj2, range);
  }

  ShapeLessArray &Add(ShapeLessArray const &obj1, type const &scalar)
  {
    assert(obj1.size() == this->size());
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x + val; },
        obj1.data());

    return *this;
  }

  ShapeLessArray &Multiply(ShapeLessArray const &obj1, ShapeLessArray const &obj2,
                           memory::Range const &range)
  {
    assert(obj1.size() == obj2.size());
    assert(obj1.size() == this->size());

    if (range.is_undefined())
    {
      Multiply(obj1, obj2);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());

      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x * y; },
          obj1.data(), obj2.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &Multiply(ShapeLessArray const &obj1, ShapeLessArray const &obj2)
  {
    memory::Range range{0, std::min(obj1.data().size(), obj2.data().size()), 1};
    return Multiply(obj1, obj2, range);
  }

  ShapeLessArray &Multiply(ShapeLessArray const &obj1, type const &scalar)
  {
    assert(obj1.size() == this->size());
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x * val; },
        obj1.data());

    return *this;
  }

  ShapeLessArray &Subtract(ShapeLessArray const &obj1, ShapeLessArray const &obj2,
                           memory::Range const &range)
  {
    assert(obj1.size() == obj2.size());
    assert(obj1.size() == this->size());

    if (range.is_undefined())
    {
      Subtract(obj1, obj2);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());

      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x - y; },
          obj1.data(), obj2.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &Subtract(ShapeLessArray const &obj1, ShapeLessArray const &obj2)
  {
    memory::Range range{0, std::min(obj1.data().size(), obj2.data().size()), 1};
    return Subtract(obj1, obj2, range);
  }

  ShapeLessArray &Subtract(ShapeLessArray const &obj1, type const &scalar)
  {
    assert(obj1.size() == this->size());
    assert(obj1.data().size() == this->data().size());

    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x - val; },
        obj1.data());

    return *this;
  }

  ShapeLessArray &Subtract(type const &scalar, ShapeLessArray const &obj1)
  {
    assert(obj1.size() == this->size());
    for (std::size_t i = 0; i < this->size(); ++i)
    {
      this->operator[](i) = scalar - obj1[i];
    }

    return *this;
  }

  ShapeLessArray &Divide(ShapeLessArray const &obj1, ShapeLessArray const &obj2,
                         memory::Range const &range)
  {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    if (range.is_undefined())
    {
      Subtract(obj1, obj2);
    }
    else if (range.is_trivial())
    {
      auto r = range.ToTrivialRange(this->data().size());

      this->data().in_parallel().Apply(
          r,
          [](vector_register_type const &x, vector_register_type const &y,
             vector_register_type &z) { z = x / y; },
          obj1.data(), obj2.data());
    }
    else
    {
      TODO_FAIL("Non-trivial ranges not implemented");
    }

    return *this;
  }

  ShapeLessArray &Divide(ShapeLessArray const &obj1, ShapeLessArray const &obj2)
  {
    memory::Range range{0, std::min(obj1.data().size(), obj2.data().size()), 1};
    return Divide(obj1, obj2, range);
  }

  ShapeLessArray &Divide(ShapeLessArray const &obj1, type const &scalar)
  {
    assert(obj1.data().size() == this->data().size());

    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x / val; },
        obj1.data());

    return *this;
  }

  ShapeLessArray &Divide(type const &scalar, ShapeLessArray const &obj1)
  {
    assert(obj1.size() == this->size());

    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = val / x; },
        obj1.data());

    return *this;
  }

  type Max() const
  {
    return data_.in_parallel().Reduce(
        memory::TrivialRange(0, size()),
        [](vector_register_type const &a, vector_register_type const &b) -> vector_register_type {
          return max(a, b);
        });
  }

  type Min() const
  {
    return data_.in_parallel().Reduce(
        memory::TrivialRange(0, size()),
        [](vector_register_type const &a, vector_register_type const &b) -> vector_register_type {
          return min(a, b);
        });
  }

  type Product() const
  {
    return data_.in_parallel().Reduce(
        memory::TrivialRange(0, size()),
        [](vector_register_type const &a, vector_register_type const &b) -> vector_register_type {
          return a * b;
        });
  }

  type Sum() const
  {
    return data_.in_parallel().Reduce(
        memory::TrivialRange(0, size()),
        [](vector_register_type const &a, vector_register_type const &b) -> vector_register_type {
          return a + b;
        });
  }

  void Fill(type const &value, memory::Range const &range)
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

  void Fill(type const &value, memory::TrivialRange const &range)
  {
    vector_register_type val(value);

    this->data().in_parallel().Apply(range, [val](vector_register_type &z) { z = val; });
  }

  void Fill(type const &value)
  {
    vector_register_type val(value);

    this->data().in_parallel().Apply([val](vector_register_type &z) { z = val; });
  }

  /**
   * calculates the product of all values in data
   * @return ret is a value of type giving the product
   */
  type &CumulativeProduct()
  {
    type ret = 1;
    for (auto cur_val : data())
    {
      ret *= cur_val;
    }
    return ret;
  }

  /**
   * calculates the sum of all values in data
   * @return ret is a value of type giving the sum
   */
  void CumulativeSum()
  {
    type ret = 0;
    for (auto cur_val : data())
    {
      ret += cur_val;
    }
    return ret;
  }

  //  type PeakToPeak() const { return Max() - Min(); }
  //
  //  void StandardDeviation(self_type const &x)
  //  {
  //    LazyResize(x.size());
  //
  //    assert(size_ > 1);
  //    kernels::StandardDeviation<type, vector_register_type> kernel(fetch::math::statistics::Mean,
  //    type(1) / type(size_)); this->data_.in_parallel().Apply(kernel, x.data());
  //  }
  //
  //  void Variance(self_type const &x)
  //  {
  //    LazyResize(x.size());
  //    assert(size_ > 1);
  //    kernels::Variance<type, vector_register_type> kernel(fetch::math::statistics::Mean, type(1)
  //    / type(size_)); this->data_.in_parallel().Apply(kernel, x.data_);
  //  }

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

  void Abs(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Abs<type> kernel;
    //    data_.in_parallel().Apply2(kernel, &kernels::stdlib::Abs< type
    //    >::operator(), x.data_);
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void ApproxSoftMax(self_type const &x)
  {
    //    kernels::ApproxSoftMax< type, vector_register_type > kernel;
    //    kernel( this->data_, x.data());
  }

  /**
   * calculates the l2loss of data in the array
   *
   * @return       returns single value as type
   *
   **/
  type L2Loss() const
  {
    type sum = data_.in_parallel().SumReduce([](vector_register_type const &v) { return v * v; });
    return sum * type(0.5);
  }

  void Fmod(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fmod<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Remainder(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Remainder<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Remquo(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Remquo<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fma(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fma<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fmax(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fmax<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fmin(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fmin<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fdim(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fdim<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nan(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nan<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nanf(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nanf<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nanl(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nanl<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Exp(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Exp<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Exp2(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Exp2<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Expm1(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Expm1<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Log(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Log<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Log10(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Log10<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Log2(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Log2<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Log1p(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Log1p<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Pow(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Pow<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Sqrt(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Sqrt<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Cbrt(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Cbrt<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Hypot(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Hypot<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Sin(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Sin<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Cos(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Cos<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Tan(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Tan<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Asin(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Asin<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Acos(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Acos<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Atan(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Atan<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Atan2(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Atan2<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Sinh(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Sinh<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Cosh(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Cosh<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Tanh(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Tanh<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Asinh(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Asinh<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Acosh(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Acosh<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Atanh(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Atanh<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Erf(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Erf<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Erfc(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Erfc<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Tgamma(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Tgamma<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Lgamma(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Lgamma<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Ceil(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Ceil<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Floor(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Floor<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Trunc(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Trunc<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Round(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Round<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Lround(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Lround<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Llround(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Llround<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nearbyint(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nearbyint<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Rint(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Rint<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Lrint(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Lrint<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Llrint(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Llrint<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Frexp(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Frexp<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Ldexp(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Ldexp<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Modf(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Modf<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Scalbn(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Scalbn<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Scalbln(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Scalbln<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Ilogb(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Ilogb<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Logb(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Logb<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nextafter(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nextafter<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Nexttoward(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Nexttoward<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Copysign(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Copysign<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Fpclassify(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Fpclassify<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isfinite(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isfinite<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isinf(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isinf<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isnan(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isnan<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isnormal(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isnormal<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Signbit(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Signbit<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isgreater(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isgreater<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isgreaterequal(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isgreaterequal<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isless(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isless<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Islessequal(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Islessequal<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Islessgreater(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Islessgreater<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void Isunordered(self_type const &x)
  {
    LazyResize(x.size());

    kernels::stdlib::Isunordered<type> kernel;
    data_.in_parallel().Apply(kernel, x.data_);
  }

  void ApproxExp(self_type const &x)
  {
    LazyResize(x.size());

    kernels::ApproxExp<vector_register_type> aexp;
    data_.in_parallel().Apply(aexp, x.data_);
  }

  void ApproxLog(self_type const &x)
  {
    LazyResize(x.size());

    kernels::ApproxLog<vector_register_type> alog;
    data_.in_parallel().Apply(alog, x.data_);
  }

  void ApproxLogistic(self_type const &x)
  {
    LazyResize(x.size());

    kernels::ApproxLogistic<vector_register_type> alog;
    data_.in_parallel().Apply(alog, x.data_);
  }

  void Relu(self_type const &x)
  {
    LazyResize(x.size());

    kernels::Relu<vector_register_type> relu;
    data_.in_parallel().Apply(relu, x.data_);
  }

  void Sign(self_type const &x)
  {
    LazyResize(x.size());

    kernels::Sign<vector_register_type> sign;
    data_.in_parallel().Apply(sign, x.data_);
  }

  /**
   * trivial implementation of softmax
   * @param x
   * @return
   */
  self_type Softmax(self_type const &x)
  {
    LazyResize(x.size());

    assert(x.size() == this->size());

    // by subtracting the max we improve numerical stability, and the result will be identical
    this->Subtract(x, x.Max());
    this->Exp(*this);
    this->Divide(*this, this->Sum());

    return *this;
  }

  /**
   * trivial implementation of maximum - return array with elementwise max applied
   * @param x
   * @return
   */
  self_type Maximum(self_type const &x, self_type const &y)
  {
    LazyResize(x.size());
    assert(x.size() == this->size());
    assert(x.size() == y.size());

    for (std::size_t i = 0; i < size(); ++i)
    {
      this->operator[](i) = std::max(x[i], y[i]);
    }

    return *this;
  }

  /**
   * calculates bit mask on this
   * @param x
   */
  void BooleanMask(self_type const &mask)
  {
    assert(this->size() == mask.size());

    std::size_t counter = 0;
    for (std::size_t i = 0; i < this->size(); ++i)
    {
      assert((mask[i] == 1) || (mask[i] == 0));
      // TODO(private issue 193): implement boolean only ndarray to avoid cast
      if (bool(mask[i]))
      {
        this->operator[](counter) = this->operator[](i);
        ++counter;
      }
    }

    LazyResize(counter);
  }

  /**
   * interleave data from multiple sources
   * @param x
   */
  void DynamicStitch(std::vector<std::vector<std::size_t>> const &indices, std::vector<self_type> const &data)
  {
    // identify the new size of this
    std::size_t new_size = 0;
    for (std::size_t l = 0; l < indices.size(); ++l)
    {
      new_size += indices[l].size();
    }
    std::cout << "new size: " << new_size << std::endl;
    LazyResize(new_size);

    // loop through all output data locations identifying the next data point to copy into it
    for (std::size_t i = 0; i < indices.size(); ++i) // iterate through lists of indices
    {
      std::cout << "i: " << i << std::endl;
      for (std::size_t k = 0; k < indices[i].size(); ++k) // iterate through index within this list
      {
        std::cout << "k: " << k << std::endl;

        std::cout << " indices[i][k]: " << indices[i][k] << std::endl;
        std::cout << " data[i][k]: " << data[i][k] << std::endl;

        assert(indices[i][k] <= this->size());
        this->operator[](indices[i][k]) = data[i][k];
      }
    }
  }

  /* Equality operator.
   * @other is the array which this instance is compared against.
   *
   * This method is sensitive to height and width.
   */
  bool operator==(ShapeLessArray const &other) const
  {
    if (size() != other.size())
    {
      return false;
    }
    bool ret = true;

    for (size_type i = 0; i < data().size(); ++i)
    {
      ret &= (data()[i] == other.data()[i]);
    }

    return ret;
  }

  /* Not-equal operator.
   * @other is the array which this instance is compared against.
   *
   * This method is sensitive to height and width.
   */
  bool operator!=(ShapeLessArray const &other) const { return !(this->operator==(other)); }

  /* One-dimensional reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that is
   * meant for non-constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, type>::type &operator[](S const &i)
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
  typename std::enable_if<std::is_integral<S>::value, type>::type const &operator[](
      S const &i) const
  {
    return data_[i];
  }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   *
   * Note this accessor is "slow" as it takes care that the developer
   * does not accidently enter the padded area of the memory.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, type>::type const &At(S const &i) const
  {
    return data_[i];
  }

  /* One-dimensional reference access function.
   * @param i is the index which is being accessed.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, type>::type &At(S const &i)
  {
    return data_[i];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, type>::type const &Set(S const &   i,
                                                                             type const &t)
  {
    return data_[i] = t;
  }

  static ShapeLessArray Arange(type const &from, type const &to, type const &delta)
  {
    ShapeLessArray ret;

    std::size_t N = std::size_t((to - from) / delta);
    ret.LazyResize(N);
    ret.SetPaddedZero();
    ret.FillArange(from, to);

    return ret;
  }

  ShapeLessArray &FillArange(type from, type const &to)
  {
    assert(from < to);

    std::size_t N     = this->size();
    type        d     = from;
    type        delta = (to - from) / static_cast<type>(N);

    for (std::size_t i = 0; i < N; ++i)
    {
      this->data()[i] = type(d);
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
      this->data()[i] = type(random::Random::generator.AsDouble());
    }
    return *this;
  }

  ShapeLessArray &FillUniformRandomIntegers(int64_t const &min, int64_t const &max)
  {
    assert(min <= max);

    uint64_t diff = uint64_t(max - min);

    for (std::size_t i = 0; i < this->size(); ++i)
    {
      this->data()[i] = type(int64_t(random::Random::generator() % diff) + min);
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
    if (other.size() != N) return false;
    bool ret = true;
    for (std::size_t i = 0; i < N; ++i)
    {
      double va = this->At(i);
      if (ignoreNaN && std::isnan(va)) continue;
      double vb = other[i];
      if (ignoreNaN && std::isnan(vb)) continue;
      double vA = (va - vb);
      if (vA < 0) vA = -vA;
      if (va < 0) va = -va;
      if (vb < 0) vb = -vb;
      double M = std::max(va, vb);

      ret &= (vA < std::max(atol, M * rtol));
    }
    if (!ret)
    {
      for (std::size_t i = 0; i < N; ++i)
      {
        double va = this->At(i);
        if (ignoreNaN && std::isnan(va)) continue;
        double vb = other[i];
        if (ignoreNaN && std::isnan(vb)) continue;
        double vA = (va - vb);
        if (vA < 0) vA = -vA;
        if (va < 0) va = -va;
        if (vb < 0) vb = -vb;
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

  iterator         begin() { return data_.begin(); }
  iterator         end() { return data_.end(); }
  reverse_iterator rbegin() { return data_.rbegin(); }
  reverse_iterator rend() { return data_.rend(); }

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

  void Set(std::size_t const &idx, type const &val) { data_[idx] = val; }

  container_type const &data() const { return data_; }
  container_type &      data() { return data_; }
  std::size_t           size() const { return size_; }

  /* Returns the capacity of the array. */
  size_type capacity() const { return data_.padded_size(); }
  size_type padded_size() const { return data_.padded_size(); }

private:
  container_type data_;
  std::size_t    size_ = 0;
};
}  // namespace math
}  // namespace fetch
