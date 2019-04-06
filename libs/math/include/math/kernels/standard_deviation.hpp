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
struct StandardDeviation
{
  StandardDeviation(type const &m, type const &r)
    : mean(m)
    , rec(r)
  {}
  void operator()(vector_register_type const &a, vector_register_type &c) const
  {
    c = a - mean;
    c = rec * c * c;
    c = sqrt(c);
  }
  vector_register_type mean;
  vector_register_type rec;
};

}  // namespace kernels
}  // namespace fetch
