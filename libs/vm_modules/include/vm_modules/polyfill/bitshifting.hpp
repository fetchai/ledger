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

#include "vm/module.hpp"

#include <cmath>

namespace fetch {
namespace vm_modules {

template <typename T>
T LeftShift(fetch::vm::VM * /*vm*/, T x, T s)
{
  return T(x << s);
}

template <typename T>
T RightShift(fetch::vm::VM * /*vm*/, T x, T s)
{
  return T(x >> s);
}

inline void BindBitShift(vm::Module &module)
{
  module.CreateFreeFunction("leftShift", &LeftShift<int32_t>);
  module.CreateFreeFunction("leftShift", &LeftShift<int64_t>);
  module.CreateFreeFunction("leftShift", &LeftShift<uint32_t>);
  module.CreateFreeFunction("leftShift", &LeftShift<uint64_t>);

  module.CreateFreeFunction("rightShift", &RightShift<int32_t>);
  module.CreateFreeFunction("rightShift", &RightShift<int64_t>);
  module.CreateFreeFunction("rightShift", &RightShift<uint32_t>);
  module.CreateFreeFunction("rightShift", &RightShift<uint64_t>);
}

}  // namespace vm_modules
}  // namespace fetch