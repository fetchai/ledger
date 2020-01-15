#pragma once
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

#include "vectorise/vectorise.hpp"
#ifdef __AVX2__
#include "vectorise/arch/avx2/math/standard_functions.hpp"
#endif
#include "vectorise/math/max.hpp"
#include "vectorise/math/min.hpp"

#include <cmath>
#include <cstddef>

namespace fetch {
namespace vectorise {

template <typename T, std::size_t S>
VectorRegister<T, S> exp(VectorRegister<T, S> x, T const &precision = 0.00001)
{
  VectorRegister<T, S> ret(T(0)), xserie(T(1));
  VectorRegister<T, S> p(precision);
  std::size_t          n = 0;

  while (any_less_than(p, Abs(xserie)))
  {
    ret = ret + xserie;
    ++n;

    VectorRegister<T, S> vecn((T(n)));
    xserie = xserie * (x / vecn);
  }

  return ret;
}

}  // namespace vectorise
}  // namespace fetch
