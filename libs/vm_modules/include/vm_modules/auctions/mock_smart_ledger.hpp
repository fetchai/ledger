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

#include "auctions/mock_smart_ledger.hpp"

struct MSL
{
 int32_t SomeFunc()
  {
    return  int32_t(0);
  }
};

void CreateSystem(std::shared_ptr<fetch::vm::Module> module)
{
  module->CreateClassType<MSL>("MockSmartLedger")
      .CreateTypeFunction("Argc", &System::Argc)
      .CreateTypeFunction("Argv", &System::Argv);
}
