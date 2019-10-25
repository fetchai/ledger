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

void OpcodeBenchmark(benchmark::State &state, std::string &ETCH_CODE, std::string const &benchmarkName, std::string const &baselineName) {
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
    if (it.opcode != fetch::vm::Opcodes::Return && it.opcode != fetch::vm::Opcodes::ReturnValue) {
      ofs << ",";
    }
  }
  ofs << std::endl;

}

// Template functions for generating Etch code for different variable types
std::string VarDec(std::string etchType) {
  return "var x : " + etchType + ";\n";
}

std::string VarDecAss(std::string etchType, std::string value) {
  return "var x : " + etchType + " = " + value + ";\n";
}

std::string ArrayDec(std::string etchType, std::string dim) {
  return "var x = Array<" + etchType + ">(" + dim + ");\n";
}

std::string IfThen(std::string condition, std::string consequent) {
  return "if (" + condition + ")\n" + consequent + "endif\n";
}

std::string IfThenElse(std::string condition, std::string consequent, std::string alternate) {
  return "if (" + condition + ")\n" + consequent + "else\n" + alternate + "endif\n";
}

std::string For(std::string expression, std::string numIter) {
  return "for (i in 0:" + numIter + ")\n" + expression + "endfor\n";
}

// Function statements and control flow
std::string FUN_MAIN ("function main()\n");
std::string FUN_USER ("function user()\n");
std::string FUN_CALL ("user();\n");
std::string END_FUN ("endfunction\n");
std::string BRK ("break;\n");
std::string CONT ("continue;\n");
std::string RETURN (FUN_MAIN + END_FUN);

// Operations
std::string ONE ("1");
std::string PUSH ("x;\n");
std::string POP ("x = x;\n");
std::string ADD ("x + x;\n");
std::string SUB ("x - x;\n");
std::string MUL ("x * x;\n");
std::string DIV ("x / x;\n");
std::string MOD ("x % x;\n");
std::string NEG ("-x;\n");
std::string EQ ("x == x;\n");
std::string NEQ ("x != x;\n");
std::string LT ("x < x;\n");
std::string GT ("x > x;\n");
std::string LTE ("x <= x;\n");
std::string GTE ("x >= x;\n");
std::string PRE_INC ("++x;\n");
std::string PRE_DEC ("--x;\n");
std::string POST_INC ("x++;\n");
std::string POST_DEC ("x--;\n");
std::string INP_ADD ("x += x;\n");
std::string INP_SUB ("x -= x;\n");
std::string INP_MUL ("x *= x;\n");
std::string INP_DIV ("x /= x;\n");
std::string INP_MOD ("x %= x;\n");

// Benchmark format: BENCHMARK_CAPTURE(Function,Name,ETCH_CODE,"Name","BaselineName");
BENCHMARK_CAPTURE(OpcodeBenchmark,Return,RETURN,"Return","Return");

// Null and Boolean benchmark codes
std::string EMPTY;
std::string TRUE ("true");
std::string FALSE ("false");
std::string PUSH_NULL (FUN_MAIN + "null;\n" + END_FUN);
std::string PUSH_FALSE (FUN_MAIN + FALSE + ";\n" + END_FUN);
std::string PUSH_TRUE (FUN_MAIN + TRUE + ";\n" + END_FUN);
std::string JUMP_IF_FALSE (FUN_MAIN + IfThen(FALSE,EMPTY) + END_FUN);
std::string JUMP (FUN_MAIN + IfThenElse(FALSE,EMPTY,EMPTY) + END_FUN);
std::string NOT (FUN_MAIN + "!true;\n" + END_FUN);
std::string FOR_LOOP (FUN_MAIN + For(EMPTY,ONE) + END_FUN);
std::string BREAK (FUN_MAIN + For(BRK,ONE) + END_FUN);
std::string CONTINUE (FUN_MAIN + For(CONT,ONE) + END_FUN);
std::string DESTRUCT_BASE (FUN_MAIN + VarDec("String") + For(EMPTY,ONE) + END_FUN);
std::string DESTRUCT (FUN_MAIN + For(VarDec("String"),ONE) + END_FUN);
std::string FUNC (FUN_MAIN + FUN_CALL + END_FUN + FUN_USER + END_FUN);

BENCHMARK_CAPTURE(OpcodeBenchmark,PushNull,PUSH_NULL,"PushNull","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushFalse,PUSH_FALSE,"PushFalse","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushTrue,PUSH_TRUE,"PushTrue","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,JumpIfFalse,JUMP_IF_FALSE,"JumpIfFalse","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,Jump,JUMP,"Jump","JumpIfFalse");
BENCHMARK_CAPTURE(OpcodeBenchmark,Not,NOT,"Not","PushTrue");
BENCHMARK_CAPTURE(OpcodeBenchmark,ForLoop,FOR_LOOP,"ForLoop","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,Break,BREAK,"Break","ForLoop");
BENCHMARK_CAPTURE(OpcodeBenchmark,Continue,CONTINUE,"Continue","ForLoop");
BENCHMARK_CAPTURE(OpcodeBenchmark,DestructBase,DESTRUCT_BASE,"DestructBase","ForLoop");
BENCHMARK_CAPTURE(OpcodeBenchmark,Destruct,DESTRUCT,"Destruct","DestructBase");
BENCHMARK_CAPTURE(OpcodeBenchmark,Function,FUNC,"Function","Return");

// String and Object benchmark codes
std::string STRING ("String");
std::string VAL_STRING ("\"x\"");
std::string PUSH_STRING (FUN_MAIN + VAL_STRING + ";\n" + END_FUN);
std::string VAR_DEC_STRING (FUN_MAIN + VarDec(STRING) + END_FUN);
std::string VAR_DEC_ASS_STRING (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + END_FUN);
std::string PUSH_VAR_STRING (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + PUSH + END_FUN);
std::string OBJ_EQ (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + EQ + END_FUN);
std::string OBJ_NEQ (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + NEQ + END_FUN);
std::string OBJ_LT (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + LT + END_FUN);
std::string OBJ_GT (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + GT + END_FUN);
std::string OBJ_LTE (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + LTE + END_FUN);
std::string OBJ_GTE (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + GTE + END_FUN);
std::string OBJ_ADD (FUN_MAIN + VarDecAss(STRING,VAL_STRING) + ADD + END_FUN);

BENCHMARK_CAPTURE(OpcodeBenchmark,PushString,PUSH_STRING,"PushString","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareString,VAR_DEC_STRING,"VariableDeclareString","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareAssignString,VAR_DEC_ASS_STRING,"VariableDeclareAssignString","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushVariableString,PUSH_VAR_STRING,"PushVariableString","VariableDeclareAssignString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectEqual,OBJ_EQ,"ObjectEqual","PushVariableString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectNotEqual,OBJ_NEQ,"ObjectNotEqual","PushVariableString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectLessThan,OBJ_LT,"ObjectLessThan","PushVariableString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectLessThanOrEqual,OBJ_LTE,"ObjectLessThanOrEqual","PushVariableString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectGreaterThan,OBJ_GT,"ObjectGreaterThan","PushVariableString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectGreaterThanOrEqual,OBJ_GTE,"ObjectGreaterThanOrEqual","PushVariableString");
BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectAdd,OBJ_ADD,"ObjectAdd","PushVariableString");

// Uint32 benchmark codes
std::string INT32 ("Int32");
std::string VAL_INT32 ("1i32");
std::string FUN_MAIN_RET_INT32 ("function main() : Int32\n");
std::string RET_VAL_INT32 (FUN_MAIN_RET_INT32 + "return " + VAL_INT32 + ";\n" + END_FUN);
std::string VAR_DEC_INT32 (FUN_MAIN + VarDec(INT32) + END_FUN);
std::string VAR_DEC_ASS_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + END_FUN);
std::string PUSH_CONST_INT32 (FUN_MAIN + VAL_INT32 + ";\n" + END_FUN);
std::string PUSH_VAR_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + PUSH + END_FUN);
std::string POP_TO_VAR_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + POP + END_FUN);
std::string PRIM_ADD_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + ADD + END_FUN);
std::string PRIM_SUB_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + SUB + END_FUN);
std::string PRIM_MUL_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + MUL + END_FUN);
std::string PRIM_DIV_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + DIV + END_FUN);
std::string PRIM_MOD_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + MOD + END_FUN);
std::string PRIM_NEG_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + NEG + END_FUN);
std::string PRIM_EQ_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + EQ + END_FUN);
std::string PRIM_NEQ_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + NEQ + END_FUN);
std::string PRIM_LT_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + LT + END_FUN);
std::string PRIM_GT_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + GT + END_FUN);
std::string PRIM_LTE_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + LTE + END_FUN);
std::string PRIM_GTE_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + GTE + END_FUN);
std::string PRIM_PRE_INC_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + PRE_INC + END_FUN);
std::string PRIM_PRE_DEC_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + PRE_DEC + END_FUN);
std::string PRIM_POST_INC_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + POST_INC + END_FUN);
std::string PRIM_POST_DEC_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + POST_DEC + END_FUN);
std::string VAR_PRIM_INP_ADD_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_ADD + END_FUN);
std::string VAR_PRIM_INP_SUB_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_SUB + END_FUN);
std::string VAR_PRIM_INP_MUL_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_MUL + END_FUN);
std::string VAR_PRIM_INP_DIV_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_DIV + END_FUN);
std::string VAR_PRIM_INP_MOD_INT32 (FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_MOD + END_FUN);
std::string ARRAY_DEC_INT32 (FUN_MAIN + ArrayDec(INT32,ONE) + END_FUN);
std::string ARRAY_IND_ADD_INT32 (FUN_MAIN + ArrayDec(INT32,ONE) + "x[0] = x[0] + " + VAL_INT32 + ";\n" + END_FUN);
std::string ARRAY_IND_INP_ADD_INT32 (FUN_MAIN + ArrayDec(INT32,ONE) + "x[0] += " + VAL_INT32 + ";\n" + END_FUN);

BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareInt32,VAR_DEC_INT32,"VariableDeclareInt32","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareAssignInt32,VAR_DEC_ASS_INT32,"VariableDeclareAssignInt32","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushConstInt32,PUSH_CONST_INT32,"PushConstInt32","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,ReturnValueInt32,RET_VAL_INT32,"ReturnValueInt32","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,PushVariableInt32,PUSH_VAR_INT32,"PushVariableInt32","PushConstInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PopToVariableInt32,POP_TO_VAR_INT32,"PopToVariableInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveAddInt32,PRIM_ADD_INT32,"PrimitiveAddInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveSubtractInt32,PRIM_SUB_INT32,"PrimitiveSubtractInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveMultiplyInt32,PRIM_MUL_INT32,"PrimitiveMultiplyInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveDivideInt32,PRIM_DIV_INT32,"PrimitiveDivideInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveModuloInt32,PRIM_MOD_INT32,"PrimitiveModuloInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveNegateInt32,PRIM_NEG_INT32,"PrimitiveNegateInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveEqualInt32,PRIM_EQ_INT32,"PrimitiveEqualInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveNotEqualInt32,PRIM_NEQ_INT32,"PrimitiveNotEqualInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveLessThanInt32,PRIM_LT_INT32,"PrimitiveLessThanInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveGreaterThanInt32,PRIM_GT_INT32,"PrimitiveGreaterThanInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveLessThanOrEqualInt32,PRIM_LTE_INT32,"PrimitiveLessThanOrEqualInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveGreaterThanOrEqualInt32,PRIM_GTE_INT32,"PrimitiveGreaterThanOrEqualInt32","PushVariableInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrefixIncInt32,PRIM_PRE_INC_INT32,"VariablePrefixIncInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrefixDecInt32,PRIM_PRE_DEC_INT32,"VariablePrefixDecInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePostfixIncInt32,PRIM_POST_INC_INT32,"VariablePostfixIncInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePostfixDecInt32,PRIM_POST_DEC_INT32,"VariablePostfixDecInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceAddInt32,VAR_PRIM_INP_ADD_INT32,"VariablePrimitiveInplaceAddInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceSubtractInt32,VAR_PRIM_INP_SUB_INT32,"VariablePrimitiveInplaceSubtractInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceMultiplyInt32,VAR_PRIM_INP_MUL_INT32,"VariablePrimitiveInplaceMultiplyInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceDivideInt32,VAR_PRIM_INP_DIV_INT32,"VariablePrimitiveInplaceDivideInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceModuloInt32,VAR_PRIM_INP_MOD_INT32,"VariablePrimitiveInplaceModuloInt32","VariableDeclareAssignInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,ArrayDeclareInt32,ARRAY_DEC_INT32,"ArrayDeclareInt32","Return");
BENCHMARK_CAPTURE(OpcodeBenchmark,ArrayIndexAddInt32,ARRAY_IND_ADD_INT32,"ArrayIndexAddInt32","ArrayDeclareInt32");
BENCHMARK_CAPTURE(OpcodeBenchmark,DuplicateInt32,ARRAY_IND_INP_ADD_INT32,"DuplicateInt32","ArrayIndexAddInt32");

// Uint8 benchmark codes
//std::string UINT8 ("UInt8");
//std::string VAL_UINT8 ("1u8");

// Uint32 benchmark codes
//std::string UINT32 ("UInt32");
//std::string VAL_UINT32 ("1u32");

// Uint64 benchmark codes
//std::string UINT64 ("UInt64");
//std::string VAL_UINT64 ("1u64");

// Fixed32 benchmark codes
//std::string FIXED32 ("Fixed32");
//std::string VAL_FIXED32 ("32.1fp32");

} // namespace

//// Function statements and control flow
//#define FUN_MAIN std::string("function main()\n")
//#define END_FUN std::string("endfunction\n")
//#define BRK std::string("break;\n")
//#define CONT std::string("continue;\n")
//#define RETURN std::string(FUN_MAIN + END_FUN)
//
//// Operations
//#define ONE std::string("1")
//#define PUSH std::string("x;\n")
//#define POP std::string("x = x;\n")
//#define ADD std::string("x + x;\n")
//#define SUB std::string("x - x;\n")
//#define MUL std::string("x * x;\n")
//#define DIV std::string("x / x;\n")
//#define MOD std::string("x % x;\n")
//#define NEG std::string("-x;\n")
//#define EQ std::string("x == x;\n")
//#define NEQ std::string("x != x;\n")
//#define LT std::string("x < x;\n")
//#define GT std::string("x > x;\n")
//#define LTE std::string("x <= x;\n")
//#define GTE std::string("x >= x;\n")
//#define PRE_INC std::string("++x;\n")
//#define PRE_DEC std::string("--x;\n")
//#define POST_INC std::string("x++;\n")
//#define POST_DEC std::string("x--;\n")
//#define INP_ADD std::string("x += x;\n")
//#define INP_SUB std::string("x -= x;\n")
//#define INP_MUL std::string("x *= x;\n")
//#define INP_DIV std::string("x /= x;\n")
//#define INP_MOD std::string("x %= x;\n")
//
//// Benchmark format: BENCHMARK_CAPTURE(Function,Name,ETCH_CODE,"Name","BaselineName");
//BENCHMARK_CAPTURE(OpcodeBenchmark,Return,RETURN,"Return","Return");
//
//// Null and Boolean benchmark codes
//#define EMPTY std::string("")
//#define TRUE std::string("true")
//#define FALSE std::string("false")
//#define PUSH_NULL std::string(FUN_MAIN + "null;\n" + END_FUN)
//#define PUSH_FALSE std::string(FUN_MAIN + FALSE + ";\n" + END_FUN)
//#define PUSH_TRUE std::string(FUN_MAIN + TRUE + ";\n" + END_FUN)
//#define JUMP_IF_FALSE std::string(FUN_MAIN + IfThen(FALSE,EMPTY) + END_FUN)
//#define JUMP std::string(FUN_MAIN + IfThenElse(FALSE,EMPTY,EMPTY) + END_FUN)
//#define NOT std::string(FUN_MAIN + "!true;\n" + END_FUN)
//#define FOR_LOOP std::string(FUN_MAIN + For(EMPTY,ONE) + END_FUN)
//#define BREAK std::string(FUN_MAIN + For(BRK,ONE) + END_FUN)
//#define CONTINUE std::string(FUN_MAIN + For(CONT,ONE) + END_FUN)
//#define DESTRUCT_BASE std::string(FUN_MAIN + VarDec("String") + For(EMPTY,ONE) + END_FUN)
//#define DESTRUCT std::string(FUN_MAIN + For(VarDec("String"),ONE) + END_FUN)
//
//BENCHMARK_CAPTURE(OpcodeBenchmark,PushNull,PUSH_NULL,"PushNull","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PushFalse,PUSH_FALSE,"PushFalse","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PushTrue,PUSH_TRUE,"PushTrue","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,JumpIfFalse,JUMP_IF_FALSE,"JumpIfFalse","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,Jump,JUMP,"Jump","JumpIfFalse");
//BENCHMARK_CAPTURE(OpcodeBenchmark,Not,NOT,"Not","PushTrue");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ForLoop,FOR_LOOP,"ForLoop","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,Break,BREAK,"Break","ForLoop");
//BENCHMARK_CAPTURE(OpcodeBenchmark,Continue,CONTINUE,"Continue","ForLoop");
//BENCHMARK_CAPTURE(OpcodeBenchmark,DestructBase,DESTRUCT_BASE,"DestructBase","ForLoop");
//BENCHMARK_CAPTURE(OpcodeBenchmark,Destruct,DESTRUCT,"Destruct","DestructBase");
//
//// String and Object benchmark codes
//#define STRING std::string("String")
//#define VAL_STRING std::string("\"x\"")
//#define PUSH_STRING std::string(FUN_MAIN + VAL_STRING + ";\n" + END_FUN)
//#define VAR_DEC_STRING std::string(FUN_MAIN + VarDec(STRING) + END_FUN)
//#define VAR_DEC_ASS_STRING std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + END_FUN)
//#define PUSH_VAR_STRING std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + PUSH + END_FUN)
//#define OBJ_EQ std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + EQ + END_FUN)
//#define OBJ_NEQ std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + NEQ + END_FUN)
//#define OBJ_LT std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + LT + END_FUN)
//#define OBJ_GT std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + GT + END_FUN)
//#define OBJ_LTE std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + LTE + END_FUN)
//#define OBJ_GTE std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + GTE + END_FUN)
//#define OBJ_ADD std::string(FUN_MAIN + VarDecAss(STRING,VAL_STRING) + ADD + END_FUN)
//
//BENCHMARK_CAPTURE(OpcodeBenchmark,PushString,PUSH_STRING,"PushString","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareString,VAR_DEC_STRING,"VariableDeclareString","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareAssignString,VAR_DEC_ASS_STRING,"VariableDeclareAssignString","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PushVariableString,PUSH_VAR_STRING,"PushVariableString","VariableDeclareAssignString");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectEqual,OBJ_EQ,"ObjectEqual","PushVariableString");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectNotEqual,OBJ_NEQ,"ObjectNotEqual","PushVariableString");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectLessThan,OBJ_LT,"ObjectLessThan","PushVariableString");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectLessThanOrEqual,OBJ_LTE,"ObjectLessThanOrEqual","PushVariableString");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectGreaterThan,OBJ_GT,"ObjectGreaterThan","PushVariableString");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectGreaterThanOrEqual,OBJ_GTE,"ObjectGreaterThanOrEqual","PushVariableString");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ObjectAdd,OBJ_ADD,"ObjectAdd","PushVariableString");
//
//// Uint32 benchmark codes
//#define INT32 std::string("Int32")
//#define VAL_INT32 std::string("1i32")
//#define FUN_MAIN_RET_INT32 std::string("function main() : Int32\n")
//#define RET_VAL_INT32 std::string(FUN_MAIN_RET_INT32 + "return " + VAL_INT32 + ";\n" + END_FUN)
//#define VAR_DEC_INT32 std::string(FUN_MAIN + VarDec(INT32) + END_FUN)
//#define VAR_DEC_ASS_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + END_FUN)
//#define PUSH_CONST_INT32 std::string(FUN_MAIN + VAL_INT32 + ";\n" + END_FUN)
//#define PUSH_VAR_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + PUSH + END_FUN)
//#define POP_TO_VAR_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + POP + END_FUN)
//#define PRIM_ADD_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + ADD + END_FUN)
//#define PRIM_SUB_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + SUB + END_FUN)
//#define PRIM_MUL_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + MUL + END_FUN)
//#define PRIM_DIV_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + DIV + END_FUN)
//#define PRIM_MOD_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + MOD + END_FUN)
//#define PRIM_NEG_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + NEG + END_FUN)
//#define PRIM_EQ_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + EQ + END_FUN)
//#define PRIM_NEQ_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + NEQ + END_FUN)
//#define PRIM_LT_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + LT + END_FUN)
//#define PRIM_GT_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + GT + END_FUN)
//#define PRIM_LTE_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + LTE + END_FUN)
//#define PRIM_GTE_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + GTE + END_FUN)
//#define PRIM_PRE_INC_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + PRE_INC + END_FUN)
//#define PRIM_PRE_DEC_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + PRE_DEC + END_FUN)
//#define PRIM_POST_INC_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + POST_INC + END_FUN)
//#define PRIM_POST_DEC_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + POST_DEC + END_FUN)
//#define VAR_PRIM_INP_ADD_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_ADD + END_FUN)
//#define VAR_PRIM_INP_SUB_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_SUB + END_FUN)
//#define VAR_PRIM_INP_MUL_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_MUL + END_FUN)
//#define VAR_PRIM_INP_DIV_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_DIV + END_FUN)
//#define VAR_PRIM_INP_MOD_INT32 std::string(FUN_MAIN + VarDecAss(INT32,VAL_INT32) + INP_MOD + END_FUN)
//#define ARRAY_DEC_INT32 std::string(FUN_MAIN + ArrayDec(INT32,ONE) + END_FUN)
//
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareInt32,VAR_DEC_INT32,"VariableDeclareInt32","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariableDeclareAssignInt32,VAR_DEC_ASS_INT32,"VariableDeclareAssignInt32","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PushConstInt32,PUSH_CONST_INT32,"PushConstInt32","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ReturnValueInt32,RET_VAL_INT32,"ReturnValueInt32","Return");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PushVariableInt32,PUSH_VAR_INT32,"PushVariableInt32","PushConstInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PopToVariableInt32,POP_TO_VAR_INT32,"PopToVariableInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveAddInt32,PRIM_ADD_INT32,"PrimitiveAddInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveSubtractInt32,PRIM_SUB_INT32,"PrimitiveSubtractInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveMultiplyInt32,PRIM_MUL_INT32,"PrimitiveMultiplyInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveDivideInt32,PRIM_DIV_INT32,"PrimitiveDivideInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveModuloInt32,PRIM_MOD_INT32,"PrimitiveModuloInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveNegateInt32,PRIM_NEG_INT32,"PrimitiveNegateInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveEqualInt32,PRIM_EQ_INT32,"PrimitiveEqualInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveNotEqualInt32,PRIM_NEQ_INT32,"PrimitiveNotEqualInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveLessThanInt32,PRIM_LT_INT32,"PrimitiveLessThanInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveGreaterThanInt32,PRIM_GT_INT32,"PrimitiveGreaterThanInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveLessThanOrEqualInt32,PRIM_LTE_INT32,"PrimitiveLessThanOrEqualInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,PrimitiveGreaterThanOrEqualInt32,PRIM_GTE_INT32,"PrimitiveGreaterThanOrEqualInt32","PushVariableInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrefixIncInt32,PRIM_PRE_INC_INT32,"VariablePrefixIncInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrefixDecInt32,PRIM_PRE_DEC_INT32,"VariablePrefixDecInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePostfixIncInt32,PRIM_POST_INC_INT32,"VariablePostfixIncInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePostfixDecInt32,PRIM_POST_DEC_INT32,"VariablePostfixDecInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceAddInt32,VAR_PRIM_INP_ADD_INT32,"VariablePrimitiveInplaceAddInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceSubtractInt32,VAR_PRIM_INP_SUB_INT32,"VariablePrimitiveInplaceSubtractInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceMultiplyInt32,VAR_PRIM_INP_MUL_INT32,"VariablePrimitiveInplaceMultiplyInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceDivideInt32,VAR_PRIM_INP_DIV_INT32,"VariablePrimitiveInplaceDivideInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,VariablePrimitiveInplaceModuloInt32,VAR_PRIM_INP_MOD_INT32,"VariablePrimitiveInplaceModuloInt32","VariableDeclareAssignInt32");
//BENCHMARK_CAPTURE(OpcodeBenchmark,ArrayDeclareInt32,ARRAY_DEC_INT32,"ArrayDeclareInt32","Return");