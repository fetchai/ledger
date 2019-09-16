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
namespace math {
namespace kernels {

template <typename vector_RegisterType>
struct Sign
{
  void operator()(vector_RegisterType const &x, vector_RegisterType &y) const
  {
    const vector_RegisterType zero(typename vector_RegisterType::type(0));
    const vector_RegisterType one(typename vector_RegisterType::type(1));
    const vector_RegisterType min_one(typename vector_RegisterType::type(-1));

    y = ((x == zero) * (zero)) + ((x > zero) * (one)) + ((x < zero) * (min_one));
  }
};

}  // namespace kernels
}  // namespace math
}  // namespace fetch
