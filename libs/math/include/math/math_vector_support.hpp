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

#include <type_traits>

namespace fetch
{
namespace math
{

template<typename T>
struct HasVectorSupport
{
  enum { value = 0 };
};

template<>
struct HasVectorSupport<double>
{
  enum { value = 1 };
};

template<>
struct HasVectorSupport<float>
{
  enum { value = 1 };
};

template<typename T, typename R>
using IfVectorSupportFor = typename std::enable_if< HasVectorSupport< T >::value, R >::type;
template<typename T, typename R>
using IfNoVectorSupportFor = typename std::enable_if< !HasVectorSupport< T >::value, R >::type;


}
}