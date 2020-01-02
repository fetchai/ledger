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

#include <cassert>
#include <vector>
namespace fetch {
namespace math {
namespace spline {

template <typename T = double>
class Spline
{
public:
  using type      = T;
  using FloatType = double;

  template <typename F>
  void SetFunction(F &f, type from, type to, std::size_t n)
  {
    assert(n < 8 * sizeof(uint64_t));
    range_from_ = from;
    range_to_   = to;
    range_span_ = range_to_ - range_from_;

    data_.resize(1u << n);
    std::size_t m   = size();
    range_to_index_ = FloatType(m - 1) / range_span_;

    type x = range_from_, delta = type(range_span_ / FloatType(m - 1));
    for (std::size_t i = 0; i < m; ++i)
    {
      data_[i] = f(x);
      x += delta;
    }
  }

  type operator()(type x)
  {
    FloatType z = (x - range_from_) * range_to_index_;
    auto      i = uint32_t(z);
    z -= FloatType(i);
    return (data_[i + 1] - data_[i]) * z + data_[i];
  }

  std::size_t size() const
  {
    return data_.size();
  }

private:
  FloatType              range_from_{}, range_to_{}, range_span_{};
  FloatType              range_to_index_{};
  FloatType              value_from_{}, value_to_{}, value_span_{};
  std::vector<FloatType> data_;
};
}  // namespace spline
}  // namespace math
}  // namespace fetch
