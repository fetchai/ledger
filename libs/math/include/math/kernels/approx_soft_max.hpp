#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

namespace fetch {
namespace kernels {

template <typename type, typename vector_register_type>
struct ApproxSoftMax
{

  template <typename array_type>
  void operator()(array_type &B, array_type const &A) const
  {
    sum_ = vector_register_type(type(0));
    B.in_parallel().Apply(*this, &ApproxSoftMax::ExponentiateAndSum, A);
    scale_ = type(1. / reduce(sum_));
    B.in_parallel().Apply(*this, &ApproxSoftMax::ScaleElements, B);
  }

  void ExponentiateAndSum(vector_register_type const &a, vector_register_type &b) const
  {
    vector_register_type e = approx_exp(a);
    sum_                   = sum_ + e;
    b                      = e;
  };

  void ScaleElements(vector_register_type const &a, vector_register_type &b) const
  {
    b = a * scale_;
  };

private:
  vector_register_type sum_;
  vector_register_type scale_;
};

}  // namespace kernels
}  // namespace fetch
