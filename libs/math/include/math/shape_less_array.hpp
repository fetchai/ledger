#pragma once
#include "core/assert.hpp"
#include "core/random.hpp"
#include "math/kernels/approx_exp.hpp"
#include "math/kernels/approx_log.hpp"
#include "math/kernels/approx_logistic.hpp"
#include "math/kernels/approx_soft_max.hpp"
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/standard_deviation.hpp"
#include "math/kernels/standard_functions.hpp"
#include "math/kernels/variance.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"

#include <algorithm>
#include <vector>

namespace fetch {
namespace math {

template <typename T, typename C = memory::SharedArray<T>>
class ShapeLessArray
{
public:
  using type = T;
  using container_type = C;
  using size_type = std::size_t;
  using vector_slice_type = typename container_type::vector_slice_type;
  using vector_register_type = typename container_type::vector_register_type;
  using vector_register_iterator_type = typename container_type::vector_register_iterator_type;

  /* Iterators for accessing and modifying the array */
  using iterator = typename container_type::iterator;
  using reverse_iterator = typename container_type::reverse_iterator;

  /* Contructs an empty shape-less array. */
  ShapeLessArray(std::size_t const &n) : data_(n), size_(n) {}

  ShapeLessArray() : data_(), size_(0) {}

  ShapeLessArray(ShapeLessArray &&other)      = default;
  ShapeLessArray(ShapeLessArray const &other) = default;
  ShapeLessArray &operator=(ShapeLessArray const &other) = default;
  ShapeLessArray &operator=(ShapeLessArray &&other) = default;

  ~ShapeLessArray() {}

  /* Set all elements to zero.
   *
   * This method will initialise all memory with zero.
   */
  void SetAllZero() { data().SetAllZero(); }

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
    assert(other.data().size() == this->data().size());

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
    assert(other.data().size() == this->data().size());

    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x + y;
        },
        this->data(), other.data());
    return *this;
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
    assert(other.data().size() == this->data().size());

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
    assert(other.data().size() == this->data().size());
    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x * y;
        },
        this->data(), other.data());
    return *this;
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
    assert(other.data().size() == this->data().size());

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
    assert(other.data().size() == this->data().size());
    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x - y;
        },
        this->data(), other.data());
    return *this;
  }

  ShapeLessArray &InlineReverseSubtract(ShapeLessArray const &other)
  {
    assert(other.data().size() == this->data().size());
    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = y - x;
        },
        this->data(), other.data());
    return *this;
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
    assert(other.data().size() == this->data().size());

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
    assert(other.data().size() == this->data().size());
    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x / y;
        },
        this->data(), other.data());
    return *this;
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

  ShapeLessArray &InlineReverseDivide(ShapeLessArray const &other)
  {
    assert(other.data().size() == this->data().size());

    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = y / x;
        },
        this->data(), other.data());
    return *this;
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
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

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
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x + y;
          /*
          alignas(16) type q[4] = {0
          };

          x.Store(q);
          std::cout << "Was here? " << q[0] << " " << q[1] << std::endl;
          y.Store(q);
          std::cout << "        + " << q[0] << " " << q[1] << std::endl;
          z.Store(q);
          std::cout << "        = " << q[0] << " " << q[1] << std::endl;
          */
        },
        obj1.data(), obj2.data());
    //    kernels::basic_aritmetics::Add<vector_register_type> kernel;
    //    this->data().in_parallel().Apply(kernel,  obj1.data(), obj2.data());

    return *this;
  }

  ShapeLessArray &Add(ShapeLessArray const &obj1, type const &scalar)
  {
    assert(obj1.data().size() == this->data().size());
    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x + val; },
        obj1.data());

    return *this;
  }

  ShapeLessArray &Multiply(ShapeLessArray const &obj1, ShapeLessArray const &obj2,
                           memory::Range const &range)
  {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

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
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x * y;
        },
        obj1.data(), obj2.data());

    return *this;
  }

  ShapeLessArray &Multiply(ShapeLessArray const &obj1, type const &scalar)
  {
    assert(obj1.data().size() == this->data().size());

    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x * val; },
        obj1.data());

    return *this;
  }

  ShapeLessArray &Subtract(ShapeLessArray const &obj1, ShapeLessArray const &obj2,
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
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x - y;
        },
        obj1.data(), obj2.data());

    return *this;
  }

  ShapeLessArray &Subtract(ShapeLessArray const &obj1, type const &scalar)
  {
    assert(obj1.data().size() == this->data().size());

    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = x - val; },
        obj1.data());

    return *this;
  }

  ShapeLessArray &Subtract(type const &scalar, ShapeLessArray const &obj1)
  {
    assert(obj1.data().size() == this->data().size());

    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = val - x; },
        obj1.data());

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
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    this->data().in_parallel().Apply(
        [](vector_register_type const &x, vector_register_type const &y, vector_register_type &z) {
          z = x / y;
        },
        obj1.data(), obj2.data());

    return *this;
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
    assert(obj1.data().size() == this->data().size());

    vector_register_type val(scalar);

    this->data().in_parallel().Apply(
        [val](vector_register_type const &x, vector_register_type &z) { z = val / x; },
        obj1.data());

    return *this;
  }

  // TODO: Move out
  type Mean() const { return Sum() / type(data_.size()); }

  type Max() const
  {
    return data_.in_parallel().Reduce(
        [](vector_register_type const &a, vector_register_type const &b) -> vector_register_type {
          return max(a, b);
        });
  }

  type Min() const
  {
    return data_.in_parallel().Reduce(
        [](vector_register_type const &a, vector_register_type const &b) -> vector_register_type {
          return min(a, b);
        });
  }

  type Product() const
  {
    return data_.in_parallel().Reduce(
        [](vector_register_type const &a, vector_register_type const &b) -> vector_register_type {
          return a * b;
        });
  }

  type Sum() const
  {
    // TODO: restrict to size
    return data_.in_parallel().Reduce(
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

  void CumlativeProduct()
  {
    // TODO: This one is easy to if done in parallel
    // but next to no speedup on a single row
  }

  void CumulativeSum()
  {
    // TODO: Same as above
  }

  type PeakToPeak() const { return Max() - Min(); }

  void StandardDeviation(self_type const &x)
  {
    LazyResize(x.size());

    assert(size_ > 1);
    kernels::StandardDeviation<type, vector_register_type> kernel(Mean(), type(1) / type(size_));
    this->data_.in_parallel().Apply(kernel, x.data());
  }

  void Variance(self_type const &x)
  {
    LazyResize(x.size());
    assert(size_ > 1);
    kernels::Variance<type, vector_register_type> kernel(Mean(), type(1) / type(size_));
    this->data_.in_parallel().Apply(kernel, x.data_);
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
    // TODO: Update vector library
    //    kernels::ApproxSoftMax< type, vector_register_type > kernel;
    //    kernel( this->data_, x.data());
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

    // TODO: Vectorize
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
  type &operator[](std::size_t const &i) { return data_[i]; }

  /* One-dimensional constant reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that can be
   * used for constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  type const &operator[](std::size_t const &i) const { return data_[i]; }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   *
   * Note this accessor is "slow" as it takes care that the developer
   * does not accidently enter the padded area of the memory.
   */
  type const &At(size_type const &i) const { return data_[i]; }

  /* One-dimensional reference access function.
   * @param i is the index which is being accessed.
   */
  type &At(size_type const &i) { return data_[i]; }

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
    // TODO: vectorise
    assert(from < to);

    std::size_t N     = this->size();
    type        d     = from;
    type        delta = (to - from) / (N - 1);

    for (std::size_t i = 0; i < N; ++i)
    {
      this->data()[i] = type(d);
      d += delta;
    }
    return *this;
  }

  static ShapeLessArray UniformRandom(std::size_t const &N)
  {
    // TODO: vectorise

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

  static ShapeLessArray Zeros(std::size_t const &n)
  {
    ShapeLessArray ret;
    ret.Resize(n);
    ret.SetAllZero();
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
      for (std::size_t i = 0; i < N; ++i) std::cout << this->At(i) << " " << other[i] << std::endl;
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

  template <typename S,
            typename D = memory::SharedArray<S>>  // TODO deduce D from parent
  void
  As(ShapeLessArray<S, D> &ret) const
  {
    ret.LazyResize(size_);
    // TODO: Vectorize
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

  // TODO: Make referenced copy

  container_type const &data() const { return data_; }
  container_type &      data() { return data_; }
  std::size_t           size() const { return size_; }

  /* Returns the capacity of the array. */
  size_type capacity() const { return data_.padded_size(); }

private:
  container_type data_;
  std::size_t    size_ = 0;
};
}  // namespace math
}  // namespace fetch
