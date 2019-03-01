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

#include "meta/type_traits.hpp"
#include <type_traits>

namespace fetch {
namespace ml {

namespace ops {
template <typename T>
class Trainable;
}  // namespace ops

namespace meta {

template <typename T, typename OperationType>
constexpr bool IsTrainable = std::is_base_of<fetch::ml::ops::Trainable<T>, OperationType>::value;

template <typename T, typename OperationType, typename R = void>
using IfIsTrainable = ::fetch::meta::EnableIf<IsTrainable<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotTrainable = ::fetch::meta::EnableIf<!IsTrainable<T, OperationType>, R>;

}  // namespace meta
}  // namespace ml
}  // namespace fetch
