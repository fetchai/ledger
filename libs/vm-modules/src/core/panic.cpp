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

void Panic(VM *vm, Ptr<String> const &s)
{
  vm->RuntimeError(s->str);
}

void Assert(VM *vm, bool condition)
{
  if (!condition)
  {
    vm->RuntimeError("Assertion error");
  }
}

void AssertWithMsg(VM *vm, bool condition, Ptr<String> const &s)
{
  if (!condition)
  {
    vm->RuntimeError("Assertion error: " + s->str);
  }
}

void CreatePanic(Module &module)
{
  module.CreateFreeFunction("panic", &Panic);
  module.CreateFreeFunction("assert", &Assert);
  module.CreateFreeFunction("assert", &AssertWithMsg);
}

}  // namespace vm_modules
}  // namespace fetch
