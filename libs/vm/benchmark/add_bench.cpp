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

#include "vm/vm.hpp"
#include "vm/module.hpp"

#include "benchmark/benchmark.h"

using fetch::vm::VM;
using fetch::vm::Module;
using fetch::vm::Executable;
using fetch::vm::Variant;

void AddInstruction(benchmark::State &state)
{
  Module module;
  VM vm{&module};

  Executable::Function func{"main", {}, 0, fetch::vm::TypeIds::Void};

  Executable::Instruction instruction{fetch::vm::Opcodes::PushTrue};
  func.AddInstruction(instruction);

  Executable executable;
  executable.AddFunction(func);

  std::string error{};
  Variant output{};
  for (auto _ : state)
  {
    vm.Execute(executable, "main", error, output);
  }
}

BENCHMARK(AddInstruction);
