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
namespace vectorise {

inline VectorRegister<float, 128> max(VectorRegister<float, 128> const &a,
                                      VectorRegister<float, 128> const &b)
{
  return VectorRegister<float, 128>(_mm_max_ps(a.data(), b.data()));
}

inline VectorRegister<double, 128> max(VectorRegister<double, 128> const &a,
                                       VectorRegister<double, 128> const &b)
{
  return VectorRegister<double, 128>(_mm_max_pd(a.data(), b.data()));
}

}  // namespace vectorise
}  // namespace fetch
