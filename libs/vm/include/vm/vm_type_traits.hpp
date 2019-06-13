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
namespace vm {
template <typename T, typename = void>
struct IsPrimitive : std::false_type
{
};
template <typename T>
struct IsPrimitive<
    T, std::enable_if_t<type_util::IsAnyOfV<T, void, bool, int8_t, uint8_t, int16_t, uint16_t,
                                            int32_t, uint32_t, int64_t, uint64_t, float, double>>>
  : std::true_type
{
};

template <typename T, typename R = void>
using IfIsPrimitive = typename std::enable_if<IsPrimitive<std::decay_t<T>>::value, R>::type;

}  // namespace vm
}  // namespace fetch
