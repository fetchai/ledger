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

#include <cstring>

#include "vm/vm.hpp"
#include "vm/module.hpp"
#include "vm/compiler.hpp"
#include "vm/ir.hpp"
#include "vm/opcodes.hpp"

#include "benchmark/benchmark.h"

using fetch::vm::VM;
using fetch::vm::Module;
using fetch::vm::Executable;
using fetch::vm::Variant;
using fetch::vm::Compiler;
using fetch::vm::IR;

namespace {

char const *EMPTY = R"(
  function main()
  endfunction
)";

char const *BASE_STRING = R"(
  //BASE_STRING
  function main()
    var x : String = "x";
    x;
  endfunction
)";

char const *EQUAL_STRING = R"(
  function main()
    var x : String = "x";
    x == x;
  endfunction
)";

char const *ADD_STRING = R"(
  function main()
    var x : String = "x";
    x + x;
  endfunction
)";

char const *BASE_UINT32 = R"(
  //BASE_UINT32
  function main()
    var x : UInt32 = 1u32;
    x;
  endfunction
)";

char const *EQUAL_UINT32 = R"(
  function main()
    var x : UInt32 = 1u32;
    x + x;
  endfunction
)";

char const *ADD_UINT32 = R"(
  function main()
    var x : UInt32 = 1u32;
    x + x;
  endfunction
)";

char const *SUB_UINT32 = R"(
  function main()
    var x : UInt32 = 1u32;
    x - x;
  endfunction
)";

char const *MUL_UINT32 = R"(
  function main()
    var x : UInt32 = 1u32;
    x * x;
  endfunction
)";

char const *DIV_UINT32 = R"(
  function main()
    var x : UInt32 = 1u32;
    x / x;
  endfunction
)";

void OpcodeBenchmark(benchmark::State &state, char const *ETCH_CODE) {
  Module module;
  Compiler compiler(&module);
  IR ir;

  // compile the source code
  std::vector<std::string> errors;
  fetch::vm::SourceFiles files = {{"default.etch", ETCH_CODE}};
  if (!compiler.Compile(files, "default_ir", ir, errors)) {
    throw std::runtime_error("Unable to compile");
  }

  // generate an IR
  Executable executable;
  VM vm{&module};
  if (!vm.GenerateExecutable(ir, "default_exe", executable, errors)) {
    throw std::runtime_error("Unable to generate IR");
  }

  auto function = executable.functions.begin();

  // benchmark iterations
  std::string error{};
  Variant output{};
  for (auto _ : state) {
    vm.Execute(executable, "main", error, output);
  }

  // write opcode lists to std::out
  std::cout << "Opcodes: ";
  std::vector<uint16_t> opcodes;
  for (auto& it : function->instructions)
  {
    opcodes.push_back(it.opcode);
    std::cout << it.opcode << " ";
  }
  std::cout << std::endl;

}

} // namespace

BENCHMARK_CAPTURE(OpcodeBenchmark,Empty,EMPTY);
BENCHMARK_CAPTURE(OpcodeBenchmark,BaseString,BASE_STRING);
BENCHMARK_CAPTURE(OpcodeBenchmark,EqualString,EQUAL_STRING);
BENCHMARK_CAPTURE(OpcodeBenchmark,AddString,ADD_STRING);
BENCHMARK_CAPTURE(OpcodeBenchmark,BaseUint32,BASE_UINT32);
BENCHMARK_CAPTURE(OpcodeBenchmark,EqualUint32,EQUAL_UINT32);
BENCHMARK_CAPTURE(OpcodeBenchmark,AddUint32,ADD_UINT32);
BENCHMARK_CAPTURE(OpcodeBenchmark,SubUint32,SUB_UINT32);
BENCHMARK_CAPTURE(OpcodeBenchmark,MulUint32,MUL_UINT32);
BENCHMARK_CAPTURE(OpcodeBenchmark,DivUint32,DIV_UINT32);