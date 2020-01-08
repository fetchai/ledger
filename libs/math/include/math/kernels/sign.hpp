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

namespace fetch {
namespace math {
namespace kernels {

template <typename VectorRegisterType>
struct Sign
{
  void operator()(VectorRegisterType const &x, VectorRegisterType &y) const
  {
    const VectorRegisterType zero(typename VectorRegisterType::type(0));
    const VectorRegisterType one(typename VectorRegisterType::type(1));
    const VectorRegisterType min_one(typename VectorRegisterType::type(-1));

    y = ((x == zero) * (zero)) + ((x > zero) * (one)) + ((x < zero) * (min_one));
  }
};

}  // namespace kernels
}  // namespace math
}  // namespace fetch
