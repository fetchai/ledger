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
#include "vm_modules/polyfill/bitwise_ops.hpp"

#include <cstdint>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

namespace {

template <typename T>
T And(VM * /*vm*/, T x, T s)
{
  return T(x & s);
}

template <typename T>
T Or(VM * /*vm*/, T x, T s)
{
  return T(x | s);
}

}  // namespace

void BindBitwiseOps(Module &module)
{
  module.CreateFreeFunction("and", &And<int32_t>);
  module.CreateFreeFunction("and", &And<int64_t>);
  module.CreateFreeFunction("and", &And<uint32_t>);
  module.CreateFreeFunction("and", &And<uint64_t>);

  module.CreateFreeFunction("or", &Or<int32_t>);
  module.CreateFreeFunction("or", &Or<int64_t>);
  module.CreateFreeFunction("or", &Or<uint32_t>);
  module.CreateFreeFunction("or", &Or<uint64_t>);
}

}  // namespace vm_modules
}  // namespace fetch
