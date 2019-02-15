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

#include <cmath>
#include <cstddef>

namespace fetch {
namespace fixed_point {
template <std::size_t I, std::size_t F>
class FixedPoint;
}
}  // namespace fetch

// These method stays in global namespace as we need them to be available just as native types abs,
// exp, log, ...
template <std::size_t I, std::size_t F>
fetch::fixed_point::FixedPoint<I, F> abs(fetch::fixed_point::FixedPoint<I, F> n)
{
  if (n < fetch::fixed_point::FixedPoint<I, F>(0))
  {
    n *= fetch::fixed_point::FixedPoint<I, F>(-1);
  }
  return n;
}

template <std::size_t I, std::size_t F>
fetch::fixed_point::FixedPoint<I, F> exp(fetch::fixed_point::FixedPoint<I, F> n)
{
  return fetch::fixed_point::FixedPoint<I, F>(::exp(double(n)));
}

template <std::size_t I, std::size_t F>
fetch::fixed_point::FixedPoint<I, F> log(fetch::fixed_point::FixedPoint<I, F> n)
{
  return fetch::fixed_point::FixedPoint<I, F>(::log(double(n)));
}
