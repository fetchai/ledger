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
#include "vm/vm.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

void Disaster(VM * /*vm*/, int32_t mode)
{
  switch (mode)
  {
  case 9:
    break;
  default:
  {
    volatile int *x{0};
    *x = 0;
  }
  }
}

void CreateDisaster(Module &module)
{
  module.CreateFreeFunction("disaster", &Disaster);
}

}  // namespace vm_modules
}  // namespace fetch
