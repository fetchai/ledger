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

/// core library ///
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/type_conversion.hpp"
#include "vm_modules/core/system.hpp"
#include "vm_modules/core/types.hpp"

namespace fetch {
namespace vm_modules {

std::shared_ptr<fetch::vm::Module> SetupModule()
{
  std::shared_ptr<fetch::vm::Module> module = std::make_shared<fetch::vm::Module>();

  // core library
  CreatePrint(module);
  CreateToString(module);
  CreateSystem(module);
  CreateIntPair(module);

  return module;
}



} // vm_modules
} // fetch
