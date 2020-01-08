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

#include "vectorise/info.hpp"
#include "vectorise/register.hpp"
#ifdef __AVX2__
#include "vectorise/arch/avx2.hpp"
#else

#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace vectorise {

ADD_REGISTER_SIZE(int32_t, 8 * sizeof(int32_t));
ADD_REGISTER_SIZE(int64_t, 8 * sizeof(int64_t));
ADD_REGISTER_SIZE(float, 8 * sizeof(float));
ADD_REGISTER_SIZE(double, 8 * sizeof(double));
ADD_REGISTER_SIZE(fixed_point::fp32_t, 8 * sizeof(fixed_point::fp32_t));
ADD_REGISTER_SIZE(fixed_point::fp64_t, 8 * sizeof(fixed_point::fp64_t));

}  // namespace vectorise
}  // namespace fetch
#endif
#include "vectorise/iterator.hpp"
