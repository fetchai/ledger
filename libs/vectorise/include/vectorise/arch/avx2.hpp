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

#ifdef __AVX2__

#include "vectorise/arch/avx2/info.hpp"

#if !defined(__clang) && (__GNUC__ < 8)
#define _mm256_set_m128i(v0, v1) _mm256_insertf128_si256(_mm256_castsi128_si256(v1), (v0), 1)
#endif

#include "vectorise/arch/avx2/register_double.hpp"
#include "vectorise/arch/avx2/register_fixed32.hpp"
#include "vectorise/arch/avx2/register_fixed64.hpp"
#include "vectorise/arch/avx2/register_float.hpp"
#include "vectorise/arch/avx2/register_int32.hpp"
#include "vectorise/arch/avx2/register_int64.hpp"

#undef ADD_REGISTER_SIZE

#endif
