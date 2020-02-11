//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/linalg/blas/gemm_tn_vector.hpp"

#include "math/linalg/blas/base.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor/tensor_view.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S>
void Blas<S, Signature(_C <= _alpha, _A, _B, _beta, _C),
          Computes(_C <= _alpha * T(_A) * _B + _beta * _C), platform::Parallelisation::VECTORISE>::
     operator()(Type const alpha, TensorView<Type> const a, TensorView<Type> const b, Type const beta,
           TensorView<Type> c) const
{
  std::size_t i;
  std::size_t j;
  if ((c.height() == 0) ||
      ((c.width() == 0) || (((alpha == Type{0}) || (a.height() == 0)) && (beta == Type{1}))))
  {
    return;
  }

  if (alpha == Type{0})
  {
    if (beta == Type{0})
    {
      for (j = 0; j < c.width(); ++j)
      {
        auto zero = Type{0};

        auto          ret_slice = c.data().slice(c.padded_height() * j, c.height());
        memory::Range range(std::size_t(0), std::size_t(c.height()));
        ret_slice.in_parallel().RangedApply(range, [zero](auto &&vw_c_j) {
          vw_c_j = static_cast<std::remove_reference_t<decltype(vw_c_j)>>(zero);
        });
      }
    }
    else
    {
      for (j = 0; j < c.width(); ++j)
      {
        auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
        auto slice_c_j = c.data().slice(c.padded_height() * std::size_t(j), c.padded_height());
        memory::Range range(std::size_t(0), std::size_t(c.height()));
        ret_slice.in_parallel().RangedApplyMultiple(
            range,
            [beta](auto const &vr_c_j, auto &vw_c_j) {
              vw_c_j = static_cast<std::remove_reference_t<decltype(vw_c_j)>>(beta) * vr_c_j;
            },
            slice_c_j);
      }
    }

    return;
  }

  for (j = 0; j < c.width(); ++j)
  {
    for (i = 0; i < c.height(); ++i)
    {
      auto temp = Type{0};

      auto slice_a_i = a.data().slice(a.padded_height() * std::size_t(i), a.padded_height());
      auto slice_b_j = b.data().slice(b.padded_height() * std::size_t(j), b.padded_height());
      memory::Range range(std::size_t(0), std::size_t(a.height()));
      temp = slice_a_i.in_parallel().SumReduce(
          range, [](auto const &vr_a_i, auto const &vr_b_j) { return vr_a_i * vr_b_j; }, slice_b_j);
      if (beta == Type{0})
      {
        c(i, j) = static_cast<Type>(alpha * temp);
      }
      else
      {
        c(i, j) = static_cast<Type>(alpha * temp + beta * c(i, j));
      }
    }
  }
}

template class Blas<int32_t, Signature(_C <= _alpha, _A, _B, _beta, _C),
                    Computes(_C <= _alpha * T(_A) * _B + _beta * _C),
                    platform::Parallelisation::VECTORISE>;

template class Blas<int64_t, Signature(_C <= _alpha, _A, _B, _beta, _C),
                    Computes(_C <= _alpha * T(_A) * _B + _beta * _C),
                    platform::Parallelisation::VECTORISE>;

template class Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
                    Computes(_C <= _alpha * T(_A) * _B + _beta * _C),
                    platform::Parallelisation::VECTORISE>;

template class Blas<float, Signature(_C <= _alpha, _A, _B, _beta, _C),
                    Computes(_C <= _alpha * T(_A) * _B + _beta * _C),
                    platform::Parallelisation::VECTORISE>;

template class Blas<
    fetch::fixed_point::FixedPoint<16, 16>, Signature(_C <= _alpha, _A, _B, _beta, _C),
    Computes(_C <= _alpha * T(_A) * _B + _beta * _C), platform::Parallelisation::VECTORISE>;

template class Blas<
    fetch::fixed_point::FixedPoint<32, 32>, Signature(_C <= _alpha, _A, _B, _beta, _C),
    Computes(_C <= _alpha * T(_A) * _B + _beta * _C), platform::Parallelisation::VECTORISE>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch
