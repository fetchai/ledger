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

#include "vm/module.hpp"
#include "vm_modules/polyfill/bitshifting.hpp"

#include <cmath>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

namespace {

template <typename T>
T LeftShift(VM * /*vm*/, T x, T s)
{
  return T(x << s);
}

template <typename T>
T RightShift(VM * /*vm*/, T x, T s)
{
  return T(x >> s);
}

}  // namespace

void BindBitShift(Module &module)
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
