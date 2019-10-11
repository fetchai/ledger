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
#include <fstream>

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

char const *RETURN = R"(
  function main()
  endfunction
)";

char const *VAR_DEC = R"(
  function main()
    var x : UInt32;
  endfunction
)";

char const *VAR_DEC_ASS = R"(
  function main()
    var x : UInt32 = 1u32;
  endfunction
)";

char const *PUSH_NULL = R"(
  function main()
    null;
  endfunction
)";

char const *PUSH_FALSE = R"(
  function main()
    false;
  endfunction
)";

char const *PUSH_TRUE = R"(
  function main()
    true;
  endfunction
)";

char const *PUSH_STRING = R"(
  function main()
    "x";
  endfunction
)";

char const *PUSH_CONST = R"(
  function main()
    "x";
  endfunction
)";

char const *PUSH_VAR = R"(
  function main()
    var x : UInt32 = 1u32;
    x;
  endfunction
)";

char const *POP_TO_VAR = R"(
  function main()
    var x : UInt32 = 1u32;
    x = x;
  endfunction
)";

char const *INC = R"(
  function main()
    var x : UInt32 = 1u32;
    x += 1u32;
  endfunction
)";

char const *PRIM_EQ = R"(
  function main()
    var x : UInt32 = 1u32;
    x == x;
  endfunction
)";

char const *PRIM_ADD = R"(
  function main()
    var x : UInt32 = 1u32;
    x + x;
  endfunction
)";

char const *PRIM_SUB = R"(
  function main()
    var x : UInt32 = 1u32;
    x - x;
  endfunction
)";

char const *PRIM_MUL = R"(
  function main()
    var x : UInt32 = 1u32;
    x * x;
  endfunction
)";

char const *PRIM_DIV = R"(
  function main()
    var x : UInt32 = 1u32;
    x / x;
  endfunction
)";

char const *PRIM_MOD = R"(
  function main()
    var x : UInt32 = 1u32;
    x % x;
  endfunction
)";

char const *PRIM_NEG = R"(
  function main()
    var x : UInt32 = 1u32;
    -x;
  endfunction
)";

char const *VAR_POST_INC = R"(
  function main()
    var x : UInt32 = 1u32;
    x++;
  endfunction
)";

char const *VAR_POST_DEC = R"(
  function main()
    var x : UInt32 = 1u32;
    x--;
  endfunction
)";

char const *NOT = R"(
  function main()
    !true;
  endfunction
)";

char const *IF = R"(
  function main()
    if (true)
    endif
  endfunction
)";

char const *ELSE = R"(
  function main()
    if (true)
    else
    endif
  endfunction
)";

char const *BASE_STRING = R"(
  function main()
    var x : String = "x";
    x;
  endfunction
)";

char const *OBJ_EQ = R"(
  function main()
    var x : String = "x";
    x == x;
  endfunction
)";

char const *OBJ_ADD = R"(
  function main()
    var x : String = "x";
    x + x;
  endfunction
)";

void OpcodeBenchmark(benchmark::State &state, char const *ETCH_CODE, std::string const &benchmarkName, std::string const &baselineName) {
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

  // write opcode lists to file
  std::ofstream ofs("opcode_lists.csv", std::ios::app);
  ofs << benchmarkName << "," << baselineName << ",";
  std::vector<uint16_t> opcodes;
  for (auto& it : function->instructions)
  {
    opcodes.push_back(it.opcode);
    ofs << it.opcode;
    if (it.opcode != fetch::vm::Opcodes::Return) {
      ofs << ",";
    }
  }
  ofs << std::endl;

}

} // namespace

// BENCHMARK_CAPTURE(Function,Name,ETCH_CODE,"Name","BaselineName");
BENCHMARK_CAPTURE(OpcodeBenchmark,Return,RETURN,"Return","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclare,VAR_DEC,"VariableDeclare","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareAssign,VAR_DEC_ASS,"VariableDeclareAssign","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushNull,PUSH_NULL,"PushNull","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushFalse,PUSH_FALSE,"PushFalse","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushTrue,PUSH_TRUE,"PushTrue","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushString,PUSH_STRING,"PushString","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushConstant,PUSH_CONST,"PushConstant","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushVariable,PUSH_VAR,"PushVariable","VariableDeclareAssign");
BENCHMARK_CAPTURE(OpcodeBenchmark,PopToVariable,POP_TO_VAR,"PopToVariable","VariableDeclareAssign");
BENCHMARK_CAPTURE(OpcodeBenchmark,Inc,INC,"Inc","VariableDeclareAssign");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveEqual,PRIM_EQ,"PrimitiveEqual","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveAdd,PRIM_ADD,"PrimitiveAdd","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveSubtract,PRIM_SUB,"PrimitiveSubtract","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveMultiply,PRIM_MUL,"PrimitiveMultiply","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveDivide,PRIM_DIV,"PrimitiveDivide","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveModulo,PRIM_MOD,"PrimitiveModulo","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveNegate,PRIM_NEG,"PrimitiveNegate","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePostfixInc,VAR_POST_INC,"VariablePostfixInc","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePostfixDec,VAR_POST_DEC,"VariablePostfixDec","PushVariable");
BENCHMARK_CAPTURE(OpcodeBenchmark,Not,NOT,"Not","PushTrue");
BENCHMARK_CAPTURE(OpcodeBenchmark,If,IF,"If","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,Else,ELSE,"Else","If");
BENCHMARK_CAPTURE(OpcodeBenchmark,BaseString,BASE_STRING,"BaseString","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectEqual,OBJ_EQ,"ObjectEqual","BaseString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectAdd,OBJ_ADD,"ObjectAdd","BaseString");