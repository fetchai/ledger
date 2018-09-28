#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
//   limitations under the License.SessionManager
//
//------------------------------------------------------------------------------

#include <iostream>

namespace fetch {
namespace ml {

template <typename ArrayType, typename FunctionType, typename LayerType>
void Backprop(ArrayType &grad, FunctionType const &back_fn, LayerType &layer)
{
  for (std::size_t i = 0; i < grad.size(); ++i)
  {
    grad[i] = 1.0;
  }

  back_fn(layer);
}

}  // namespace ml
}  // namespace fetch