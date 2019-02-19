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

#include "vm/node.hpp"
#include "vm/string.hpp"

#include <iostream>

namespace fetch {
namespace vm {

// VM free functions. These will be available for all smart contracts and MUST NOT RETAIN STATE (be
// thread safe)

fetch::vm::Ptr<fetch::vm::String> toString(fetch::vm::VM *vm, int32_t const &a)
{
  fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm, std::to_string(a)));
  return ret;
}

static void Print(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str << std::endl;
}

}  // namespace vm
}  // namespace fetch
