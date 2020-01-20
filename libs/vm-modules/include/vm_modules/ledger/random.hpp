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

#include "vm_modules/ledger/forward_decl.hpp"

#include <random>

namespace fetch {
namespace vm_modules {
namespace ledger {

class RandomUniform : public vm::Object
{
public:
  RandomUniform(vm::VM *vm, vm::TypeId type_id);

  static void Bind(vm::Module &module);

  int32_t rand(int32_t low, int32_t high);

private:
  std::random_device random_device_;
  std::mt19937 generator_;
  uint32_t reset_;
  uint32_t counter_;
};

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
