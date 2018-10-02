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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"

#include "math/free_functions/free_functions.hpp"
#include "math/rectangular_array.hpp"

#include "vectorise/threading/pool.hpp"

#include <iostream>
#include <limits>
#include <vector>

namespace fetch {
namespace math {
namespace linalg {

template <typename T, typename C = fetch::memory::SharedArray<T>,
          typename S = fetch::math::RectangularArray<T, C>>
class Matrix : public S
{
public:
  using super_type                    = S;
  using type                          = typename super_type::type;
  using vector_register_type          = typename super_type::vector_register_type;
  using vector_register_iterator_type = typename super_type::vector_register_iterator_type;
  using working_memory_2d_type        = RectangularArray<T, C, true, true>;
  using size_type                     = typename super_type::size_type;

  using self_type = Matrix<T, C, S>;

  enum
  {
    E_SIMD_BLOCKS = super_type::container_type::E_SIMD_COUNT
  };

  Matrix()                    = default;
  Matrix(Matrix &&other)      = default;
  Matrix(Matrix const &other) = default;
  Matrix &operator=(Matrix const &other) = default;
  Matrix &operator=(Matrix &&other) = default;

  Matrix(super_type const &other)
    : super_type(other)
  {}
  Matrix(super_type &&other)
    : super_type(std::move(other))
  {}
  Matrix(byte_array::ConstByteArray const &c)
  {
    std::size_t       n = 1;
    std::vector<type> elems;
    elems.reserve(1024);
    bool failed = false;

    for (uint64_t i = 0; i < c.size();)
    {
      uint64_t last = i;
      switch (c[i])
      {
      case ';':
        ++n;
        ++i;
        break;
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
          elems.push_back(type(atof(c.char_pointer() + last)));
        }
        break;
      }
    }
    std::size_t m = elems.size() / n;

    if ((m * n) != elems.size())
    {
      failed = true;
    }

    if (!failed)
    {
      this->Resize(n, m);
      this->SetAllZero();

      std::size_t k = 0;
      for (std::size_t i = 0; i < n; ++i)
      {
        for (std::size_t j = 0; j < m; ++j)
        {
          this->Set(i, j, elems[k++]);
        }
      }
    }
  }

  Matrix(std::size_t const &h, std::size_t const &w)
    : super_type(h, w)
  {}

  /**
   * for compatibility with other vectors this constructor is nice to have
   * @param shape
   */
  Matrix(std::vector<std::size_t> const &shape)
    : Matrix(shape[0], shape[1])
  {}

  Matrix(self_type &other)
  {
    this->LazyResize(other.height(), other.width());
    this->data() = other.data().Copy();
  }

  static Matrix Zeroes(std::size_t const &n, std::size_t const &m)
  {
    Matrix ret;
    ret.LazyResize(n, m);
    ret.data().SetAllZero();
    return ret;
  }
  static Matrix Zeroes(std::vector<std::size_t> const &shape)
  {
    return Zeroes(shape[0], shape[1]);
  }

  static Matrix UniformRandom(std::size_t const &n, std::size_t const &m)
  {
    Matrix ret;

    ret.LazyResize(n, m);
    ret.FillUniformRandom();

    ret.SetPaddedZero();

    return ret;
  }
  static Matrix UniformRandom(std::vector<std::size_t> const &shape)
  {
    return UniformRandom(shape[0], shape[1]);
  }
  template <typename F>
  void SetAll(F &val)
  {
    for (auto &a : *this)
    {
      a = val;
    }
  }

  template <typename G>
  self_type &Transpose(G const &other)
  {
    this->LazyResize(other.width(), other.height());

    for (std::size_t i = 0; i < other.height(); ++i)
    {
      for (std::size_t j = 0; j < other.width(); ++j)
      {
        this->At(j, i) = other.At(i, j);
      }
    }

    return *this;
  }

  /** Efficient vectorised and threaded dot routine to compute C = Dot(A, B).
   * @A is the first matrix.
   * @B is the second matrix.
   **/
  self_type &Dot(self_type const &A, self_type const &B, type alpha = 1.0, type beta = 0.0)
  {
    this->Resize(A.height(), B.width());
    fetch::math::Dot(A, B, *this, alpha, beta);
    return *this;
  }

  /**
   * Efficient vectorised and threaded routine for C = A.T(B)
   * @param A
   * @param B
   * @return
   */
  self_type &DotTranspose(self_type const &A, self_type const &B, type alpha = 1.0, type beta = 0.0)
  {
    this->Resize(A.height(), B.height());
    fetch::math::DotTranspose(A, B, *this, alpha, beta);
    return *this;
  }

  /**
   * Efficient vectorised and threaded routine for C = T(A).B
   * @param A
   * @param B
   * @return
   */
  self_type &TransposeDot(self_type const &A, self_type const &B, type alpha = 1.0, type beta = 0.0)
  {
    this->Resize(A.width(), B.width());
    fetch::math::TransposeDot(A, B, *this, alpha, beta);
    return *this;
  }
};
}  // namespace linalg
}  // namespace math
}  // namespace fetch
