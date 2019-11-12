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

#include <cmath>
#include <fstream>
#include <algorithm>

#include <vm_modules/math/random.hpp>
#include <vm_modules/math/math.hpp>
#include <vm_modules/math/bignumber.hpp>
#include <vm_modules/crypto/sha256.hpp>
#include <vm_modules/core/byte_array_wrapper.hpp>

#include "benchmark/benchmark.h"

#include "vm/vm.hpp"
#include "vm/module.hpp"
#include "vm/compiler.hpp"
#include "vm/ir.hpp"
#include "vm/opcodes.hpp"

using fetch::vm::VM;
using fetch::vm::Module;
using fetch::vm::Executable;
using fetch::vm::Variant;
using fetch::vm::Compiler;
using fetch::vm::IR;

namespace {

// Benchmark categories can be selectively suppressed using environment variables
const bool
    run_basic = std::getenv("FETCH_VM_BENCHMARK_NO_BASIC") == nullptr,
    run_object = std::getenv("FETCH_VM_BENCHMARK_NO_OBJECT") == nullptr,
    run_prim_ops = std::getenv("FETCH_VM_BENCHMARK_NO_PRIM_OPS") == nullptr,
    run_math = std::getenv("FETCH_VM_BENCHMARK_NO_MATH") == nullptr,
    run_array = std::getenv("FETCH_VM_BENCHMARK_NO_ARRAY") == nullptr,
    run_tensor = std::getenv("FETCH_VM_BENCHMARK_NO_TENSOR") == nullptr,
    run_crypto = std::getenv("FETCH_VM_BENCHMARK_NO_CRYPTO") == nullptr;

// Number of benchmarks in each category
const u_int
    n_basic_bms = 16,
    n_object_bms = 10,
    n_prim_bms = 25,
    n_math_bms = 16,
    n_array_bms = 10,
    n_tensor_bms = 4,
    n_crypto_bms = 4;

// Benchmark parameters
const u_int
    n_primitives = 12,
    n_dec_primitives = 4,
    max_array_len = 16384,
    n_array_lens = 32,
    max_str_len = 16384,
    n_str_lens = 10,
    max_tensor_size = 16777216,
    n_tensor_sizes = 16,
    n_tensor_dim_sizes = n_tensor_sizes*3;

// Index benchmarks for interpretation by "scripts/benchmark/opcode_timing.py"
const u_int
    basic_begin = 0,
    basic_end = run_basic ? n_basic_bms : 1, // always run "Return" benchmark as baseline
    object_begin = basic_end,
    object_end = object_begin + n_str_lens * n_object_bms,
    prim_begin = object_end,
    prim_end = prim_begin + n_prim_bms * n_primitives,
    math_begin = prim_end,
    math_end = math_begin + n_math_bms * n_dec_primitives,
    array_begin = math_end,
    array_end = array_begin + n_array_bms * n_array_lens,
    tensor_begin = array_end,
    tensor_end = tensor_begin + n_tensor_bms * n_tensor_dim_sizes,
    crypto_begin = tensor_end,
    crypto_end = crypto_begin + n_crypto_bms - 1 + n_str_lens;


// Main benchmark function - compiles and runs Etch code snippets and saves opcodes to file
void EtchCodeBenchmark(benchmark::State &state, std::string const &benchmark_name, std::string const &etch_code,
                       std::string const &baseline_name, u_int const bm_ind) {

  Module module;

  fetch::vm_modules::math::BindMath(module);
  fetch::vm_modules::math::BindRand(module);
  fetch::vm_modules::ByteArrayWrapper::Bind(module);
  fetch::vm_modules::math::UInt256Wrapper::Bind(module);
  fetch::vm_modules::SHA256Wrapper::Bind(module);
  Compiler compiler(&module);
  IR ir;

  // Compile the source code
  std::vector<std::string> errors;
  fetch::vm::SourceFiles files = {{"default.etch", etch_code}};
  if (!compiler.Compile(files, "default_ir", ir, errors)) {
    std::cout << "Skipping benchmark (unable to compile): " << benchmark_name << std::endl;
    return;
  }

  // Generate an IR
  Executable executable;
  VM vm{&module};
  if (!vm.GenerateExecutable(ir, "default_exe", executable, errors)) {
    std::cout << "Skipping benchmark (unable to generate IR)" << std::endl;
    return;
  }

  // Benchmark iterations
  std::string error{};
  Variant output{};
  for (auto _ : state) {
    vm.Execute(executable, "main", error, output);
  }

  auto function = executable.FindFunction("main");

  // Write opcode lists to file
  std::ofstream ofs("opcode_lists.csv", std::ios::app);
  ofs << bm_ind << "," << benchmark_name << "," << baseline_name << ",";
  for (auto &it : function->instructions) {
    ofs << it.opcode;
    if (it.opcode != fetch::vm::Opcodes::Return && it.opcode != fetch::vm::Opcodes::ReturnValue) {
      ofs << ",";
    }
  }
  ofs << std::endl;
}

// Functions for generating Etch code
std::string FunMain(std::string const &contents) {
  return "function main()\n" + contents + "endfunction\n";
}

std::string FunMainRet(std::string const &contents, std::string const &return_type) {
  return "function main() : " + return_type + "\n" + contents + "endfunction\n";
}

std::string FunUser(std::string const &contents) {
  return "function user()\n" + contents + "endfunction\n";
}

std::string VarDec(std::string const &etchType) {
  return "var x : " + etchType + ";\n";
}

std::string VarDecAss(std::string const &etchType, std::string const &value) {
  return "var x : " + etchType + " = " + value + ";\n";
}

std::string IfThen(std::string const &condition, std::string const &consequent) {
  return "if (" + condition + ")\n" + consequent + "endif\n";
}

std::string IfThenElse(std::string const &condition, std::string const &consequent, std::string const &alternate) {
  return "if (" + condition + ")\n" + consequent + "else\n" + alternate + "endif\n";
}

std::string For(std::string const &expression, std::string const &numIter) {
  return "for (i in 0:" + numIter + ")\n" + expression + "endfor\n";
}

std::string Rand(std::string const &min, std::string const &max) {
  return "rand(" + min + "," + max + ");\n";
}

std::string ArrayDec(std::string const &arr, std::string const &prim, std::string const &dim) {
  return "var " + arr + " = Array<" + prim + ">(" + dim + ");\n";
}

std::string ArrayAss(std::string const &arr, std::string const &ind, std::string const &val) {
  return arr + "[" + ind + "] = " + val + ";\n";
}

std::string ArrayAppend(std::string const &arr, std::string const &val) {
  return arr + ".append(" + val + ");\n";
}

std::string ArrayExtend(std::string const &arr1, std::string const &arr2) {
  return arr1 + ".extend(" + arr2 + ");\n";
}

std::string ArrayErase(std::string const &arr, std::string const &ind) {
  return arr + ".erase(" + ind + ");\n";
}

std::string TensorDec(std::string const &tensor, std::string const &prim, std::string const &tensor_shape,
                      std::string const &tensor_size, const u_int tensor_dim) {
  auto tensor_dec = ArrayDec(tensor_shape, prim, std::to_string(tensor_dim));
  for (u_int i = 0; i != tensor_dim; ++i) {
    tensor_dec += ArrayAss(tensor_shape, std::to_string(i), tensor_size);
  }
  tensor_dec += "var " + tensor + " = Tensor(" + tensor_shape + ");\n";
  return tensor_dec;
}

std::string Sha256Update(u_int str_len) {
  std::string str(str_len, '0');
  return "s.update(\"" + str + "\");\n";
}

/* Create a linear spaced range vector from max/n_elem to max of primitive type T
   * @max is the range maximum and last element in the vector
   *
   * @n_elem is the number of elements to include in the vector
   *
   * @return the range vector
   */
template<typename T>
std::vector<T> LinearRangeVector(T max, u_int n_elem) {
  float current_val = 0.0f;
  auto const step = static_cast<float>(max) / static_cast<float>(n_elem);
  std::vector<T> v(n_elem);
  for (auto it = v.begin(); it != v.end(); ++it) {
    current_val += step;
    *it = static_cast<T>(current_val);
  }
  return v;
}

void BasicBenchmarks(benchmark::State &state) {

  // Functions, variable declarations, and constants
  static std::string
      FUN_CALL = "user();\n",
      BRK = "break;\n",
      CONT = "continue;\n",
      ONE = "1",
      TRUE = "true",
      FALSE = "false",
      STRING = "String",
      VAL_STRING = "\"x\"",
      EMPTY;

  // Boolean and type-neutral benchmark codes
  static std::pair<std::string, std::string>
      RETURN = std::make_pair("Return", FunMain("")),
      PUSH_NULL = std::make_pair("PushNull", FunMain("null;\n")),
      PUSH_FALSE = std::make_pair("PushFalse", FunMain(FALSE + ";\n")),
      PUSH_TRUE = std::make_pair("PushTrue", FunMain(TRUE + ";\n")),
      JUMP_IF_FALSE = std::make_pair("JumpIfFalse", FunMain(IfThen(FALSE, EMPTY))),
      JUMP = std::make_pair("Jump", FunMain(IfThenElse(FALSE, EMPTY, EMPTY))),
      NOT = std::make_pair("Not", FunMain("!true;\n")),
      AND = std::make_pair("And", FunMain("true && true;\n")),
      OR = std::make_pair("Or", FunMain("false || true ;\n")),
      FOR_LOOP = std::make_pair("ForLoop", FunMain(For(EMPTY, ONE))),
      BREAK = std::make_pair("Break", FunMain(For(BRK, ONE))),
      CONTINUE = std::make_pair("Continue", FunMain(For(CONT, ONE))),
      DESTRUCT_BASE = std::make_pair("DestructBase", FunMain(VarDec(STRING) + For(EMPTY, ONE))),
      DESTRUCT = std::make_pair("Destruct", FunMain(For(VarDec(STRING), ONE))),
      FUNC = std::make_pair("Function", FunMain(FUN_CALL) + FunUser("")),
      VAR_DEC_STRING = std::make_pair("VariableDeclareStr", FunMain(VarDec(STRING)));

  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"Return",                "Return"},
                      {"PushNull",              "Return"},
                      {"PushFalse",             "Return"},
                      {"PushTrue",              "Return"},
                      {"JumpIfFalse",           "Return"},
                      {"Jump",                  "JumpIfFalse"},
                      {"Not",                   "PushTrue"},
                      {"And",                   "PushTrue"},
                      {"Or",                    "PushTrue"},
                      {"ForLoop",               "Return"},
                      {"Break",                 "ForLoop"},
                      {"Continue",              "ForLoop"},
                      {"DestructBase",          "ForLoop"},
                      {"Destruct",              "DestructBase"},
                      {"Function",              "Return"},
                      {"VariableDeclareStr",    "Return"}
                  });

  std::vector<std::pair<std::string, std::string>> const
      etch_codes = {RETURN, PUSH_NULL, PUSH_FALSE, PUSH_TRUE, JUMP_IF_FALSE, JUMP, NOT, AND, OR, FOR_LOOP, BREAK,
                    CONTINUE, DESTRUCT_BASE, DESTRUCT, FUNC, VAR_DEC_STRING};

  auto bm_ind = static_cast<u_int>(state.range(0));

  u_int const etch_ind = bm_ind - basic_begin;

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void ObjectBenchmarks(benchmark::State &state) {

  auto bm_ind = static_cast<u_int>(state.range(0));

  // Generate the string lengths from benchmark parameters
  std::vector<u_int> str_len = LinearRangeVector<u_int>(max_str_len, n_str_lens);

  // Get the indices of the string length and etch code corresponding to the benchmark range variable
  const u_int len_ind = (bm_ind - object_begin) / n_object_bms;
  const u_int etch_ind = (bm_ind - object_begin) % n_object_bms;

  std::string const str("\"" + std::string(str_len[len_ind], '0') + "\"");

  std::string const length(std::to_string(str_len[len_ind]));

  static std::string const
      STRING = "String",
      PUSH = "x;\n",
      ADD = "x + x;\n",
      EQ = "x == x;\n",
      NEQ = "x != x;\n",
      LT = "x < x;\n",
      GT = "x > x;\n",
      LTE = "x <= x;\n",
      GTE = "x >= x;\n";

  const std::pair<std::string, std::string>
      PUSH_STRING = std::make_pair("PushString_" + length, FunMain(str + ";\n")),
      VAR_DEC_ASS_STRING = std::make_pair("VariableDeclareAssignString_" + length, FunMain(VarDecAss(STRING, str))),
      PUSH_VAR_STRING = std::make_pair("PushVariableString_" + length, FunMain(VarDecAss(STRING, str) + PUSH)),
      OBJ_EQ = std::make_pair("ObjectEqualString_" + length, FunMain(VarDecAss(STRING, str) + EQ)),
      OBJ_NEQ = std::make_pair("ObjectNotEqualString_" + length, FunMain(VarDecAss(STRING, str) + NEQ)),
      OBJ_LT = std::make_pair("ObjectLessThanString_" + length, FunMain(VarDecAss(STRING, str) + LT)),
      OBJ_GT = std::make_pair("ObjectGreaterThanString_" + length, FunMain(VarDecAss(STRING, str) + GT)),
      OBJ_LTE = std::make_pair("ObjectLessThanOrEqualString_" + length, FunMain(VarDecAss(STRING, str) + LTE)),
      OBJ_GTE = std::make_pair("ObjectGreaterThanOrEqualString_" + length, FunMain(VarDecAss(STRING, str) + GTE)),
      OBJ_ADD = std::make_pair("ObjectAddString_" + length, FunMain(VarDecAss(STRING, str) + ADD));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"PushString_" + length,                     "Return"},
                      {"VariableDeclareAssignString_" + length,    "Return"},
                      {"PushVariableString_" + length,             "VariableDeclareAssignString_" + length},
                      {"ObjectEqualString_" + length,              "PushVariableString_" + length},
                      {"ObjectNotEqualString_" + length,           "PushVariableString_" + length},
                      {"ObjectLessThanString_" + length,           "PushVariableString_" + length},
                      {"ObjectLessThanOrEqualString_" + length,    "PushVariableString_" + length},
                      {"ObjectGreaterThanString_" + length,        "PushVariableString_" + length},
                      {"ObjectGreaterThanOrEqualString_" + length, "PushVariableString_" + length},
                      {"ObjectAddString_" + length,                "PushVariableString_" + length}
                  });

  std::vector<std::pair<std::string, std::string>> const
      etch_codes = {PUSH_STRING, VAR_DEC_ASS_STRING, PUSH_VAR_STRING, OBJ_EQ, OBJ_NEQ, OBJ_LT, OBJ_GT,
                    OBJ_LTE, OBJ_GTE, OBJ_ADD};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void PrimitiveOpBenchmarks(benchmark::State &state) {

  static std::vector<std::string> primitives{"Int8", "Int16", "Int32", "Int64", "UInt8", "UInt16", "UInt32", "UInt64",
                                             "Float32", "Float64", "Fixed32", "Fixed64"};

  static std::vector<std::string> values{"1i8", "1i16", "1i32", "1i64", "1u8", "1u16", "1u32", "1u64",
                                         "0.5f", "0.5", "0.5fp32", "0.5fp64"};

  auto bm_ind = static_cast<u_int>(state.range(0));

  // Get the index of the primitive corresponding to the benchmark range variable and the relevant etch code
  const u_int prim_ind = (bm_ind - prim_begin) / n_prim_bms;
  const u_int etch_ind = (bm_ind - prim_begin) % n_prim_bms;

  std::string const prim(primitives[prim_ind]);
  std::string const val(values[prim_ind]);

  // Operations
  static std::string
      PUSH = "x;\n",
      POP = "x = x;\n",
      ADD = "x + x;\n",
      SUB = "x - x;\n",
      MUL = "x * x;\n",
      DIV = "x / x;\n",
      MOD = "x % x;\n",
      NEG = "-x;\n",
      EQ = "x == x;\n",
      NEQ = "x != x;\n",
      LT = "x < x;\n",
      GT = "x > x;\n",
      LTE = "x <= x;\n",
      GTE = "x >= x;\n",
      PRE_INC = "++x;\n",
      PRE_DEC = "--x;\n",
      POST_INC = "x++;\n",
      POST_DEC = "x--;\n",
      INP_ADD = "x += x;\n",
      INP_SUB = "x -= x;\n",
      INP_MUL = "x *= x;\n",
      INP_DIV = "x /= x;\n",
      INP_MOD = "x %= x;\n";

  const std::pair<std::string, std::string>
      RET_VAL = std::make_pair("PrimReturnValue_" + prim, FunMainRet("return " + val + ";\n", prim)),
      VAR_DEC = std::make_pair("PrimVariableDeclare_" + prim, FunMain(VarDec(prim))),
      VAR_DEC_ASS = std::make_pair("PrimVariableDeclareAssign_" + prim, FunMain(VarDecAss(prim, val))),
      PUSH_CONST = std::make_pair("PrimPushConst_" + prim, FunMain(val + ";\n")),
      PUSH_VAR = std::make_pair("PrimPushVariable_" + prim, FunMain(VarDecAss(prim, val) + PUSH)),
      POP_TO_VAR = std::make_pair("PrimPopToVariable_" + prim, FunMain(VarDecAss(prim, val) + POP)),
      PRIM_ADD = std::make_pair("PrimAdd_" + prim, FunMain(VarDecAss(prim, val) + ADD)),
      PRIM_SUB = std::make_pair("PrimSubtract_" + prim, FunMain(VarDecAss(prim, val) + SUB)),
      PRIM_MUL = std::make_pair("PrimMultiply_" + prim, FunMain(VarDecAss(prim, val) + MUL)),
      PRIM_DIV = std::make_pair("PrimDivide_" + prim, FunMain(VarDecAss(prim, val) + DIV)),
      PRIM_MOD = std::make_pair("PrimModulo_" + prim, FunMain(VarDecAss(prim, val) + MOD)),
      PRIM_NEG = std::make_pair("PrimNegate_" + prim, FunMain(VarDecAss(prim, val) + NEG)),
      PRIM_EQ = std::make_pair("PrimEqual_" + prim, FunMain(VarDecAss(prim, val) + EQ)),
      PRIM_NEQ = std::make_pair("PrimNotEqual_" + prim, FunMain(VarDecAss(prim, val) + NEQ)),
      PRIM_LT = std::make_pair("PrimLessThan_" + prim, FunMain(VarDecAss(prim, val) + LT)),
      PRIM_GT = std::make_pair("PrimGreaterThan_" + prim, FunMain(VarDecAss(prim, val) + GT)),
      PRIM_LTE = std::make_pair("PrimLessThanOrEqual_" + prim, FunMain(VarDecAss(prim, val) + LTE)),
      PRIM_GTE = std::make_pair("PrimGreaterThanOrEqual_" + prim, FunMain(VarDecAss(prim, val) + GTE)),
      PRIM_PRE_INC = std::make_pair("PrimVariablePrefixInc_" + prim, FunMain(VarDecAss(prim, val) + PRE_INC)),
      PRIM_PRE_DEC = std::make_pair("PrimVariablePrefixDec_" + prim, FunMain(VarDecAss(prim, val) + PRE_DEC)),
      PRIM_POST_INC = std::make_pair("PrimVariablePostfixInc_" + prim, FunMain(VarDecAss(prim, val) + POST_INC)),
      PRIM_POST_DEC = std::make_pair("PrimVariablePostfixDec_" + prim, FunMain(VarDecAss(prim, val) + POST_DEC)),
      VAR_PRIM_INP_ADD = std::make_pair("PrimVariableInplaceAdd_" + prim, FunMain(VarDecAss(prim, val) + INP_ADD)),
      VAR_PRIM_INP_SUB = std::make_pair("PrimVariableInplaceSubtract_" + prim, FunMain(VarDecAss(prim, val) + INP_SUB)),
      VAR_PRIM_INP_MUL = std::make_pair("PrimVariableInplaceMultiply_" + prim, FunMain(VarDecAss(prim, val) + INP_MUL)),
      VAR_PRIM_INP_DIV = std::make_pair("PrimVariableInplaceDivide_" + prim, FunMain(VarDecAss(prim, val) + INP_DIV)),
      VAR_PRIM_INP_MOD = std::make_pair("PrimVariableInplaceModulo_" + prim, FunMain(VarDecAss(prim, val) + INP_MOD));

//Define{benchmark,baseline}pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"PrimReturnValue_" + prim,             "Return"},
                      {"PrimVariableDeclare_" + prim,         "Return"},
                      {"PrimVariableDeclareAssign_" + prim,   "Return"},
                      {"PrimPushConst_" + prim,               "Return"},
                      {"PrimPushVariable_" + prim,            "PrimPushConst_" + prim},
                      {"PrimPopToVariable_" + prim,           "PrimVariableDeclareAssign_" + prim},
                      {"PrimAdd_" + prim,                     "PrimPushVariable_" + prim},
                      {"PrimSubtract_" + prim,                "PrimPushVariable_" + prim},
                      {"PrimMultiply_" + prim,                "PrimPushVariable_" + prim},
                      {"PrimDivide_" + prim,                  "PrimPushVariable_" + prim},
                      {"PrimModulo_" + prim,                  "PrimPushVariable_" + prim},
                      {"PrimNegate_" + prim,                  "PrimPushVariable_" + prim},
                      {"PrimEqual_" + prim,                   "PrimPushVariable_" + prim},
                      {"PrimNotEqual_" + prim,                "PrimPushVariable_" + prim},
                      {"PrimLessThan_" + prim,                "PrimPushVariable_" + prim},
                      {"PrimGreaterThan_" + prim,             "PrimPushVariable_" + prim},
                      {"PrimLessThanOrEqual_" + prim,         "PrimPushVariable_" + prim},
                      {"PrimGreaterThanOrEqual_" + prim,      "PrimPushVariable_" + prim},
                      {"PrimVariablePrefixInc_" + prim,       "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariablePrefixDec_" + prim,       "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariablePostfixInc_" + prim,      "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariablePostfixDec_" + prim,      "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariableInplaceAdd_" + prim,      "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariableInplaceSubtract_" + prim, "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariableInplaceMultiply_" + prim, "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariableInplaceDivide_" + prim,   "PrimVariableDeclareAssign_" + prim},
                      {"PrimVariableInplaceModulo_" + prim,   "PrimVariableDeclareAssign_" + prim}
                  });

  std::vector<std::pair<std::string, std::string>> const
      etch_codes = {RET_VAL, VAR_DEC, VAR_DEC_ASS, PUSH_CONST, PUSH_VAR, POP_TO_VAR, PRIM_ADD, PRIM_SUB, PRIM_MUL,
                    PRIM_DIV, PRIM_NEG, PRIM_EQ, PRIM_NEQ, PRIM_LT, PRIM_GT, PRIM_LTE, PRIM_GTE, PRIM_PRE_INC,
                    PRIM_PRE_DEC, PRIM_POST_INC, PRIM_POST_DEC, VAR_PRIM_INP_ADD, VAR_PRIM_INP_SUB, VAR_PRIM_INP_MUL,
                    VAR_PRIM_INP_DIV, VAR_PRIM_INP_MOD};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void MathBenchmarks(benchmark::State &state) {

  static std::vector<std::string> 
      primitives{"Float32", "Float64", "Fixed32", "Fixed64"},
      values{"0.5f", "0.5", "0.5fp32", "0.5fp64"},
      alt_values{"1.5f", "1.5", "1.5fp32", "1.5fp64"};

  auto bm_ind = static_cast<u_int>(state.range(0));

  // Get the index of the math function corresponding to the benchmark range variable and the relevant etch code
  const u_int prim_ind = (bm_ind - math_begin) / n_math_bms;
  const u_int etch_ind = (bm_ind - math_begin) % n_math_bms;

  const std::string 
      prim = primitives[prim_ind],
      val = values[prim_ind],
      alt_val = alt_values[prim_ind];

  // Operations
  static std::string
      PUSH = "x;\n",
      ABS = "abs(x);\n",
      SIN = "sin(x);\n",
      COS = "cos(x);\n",
      TAN = "tan(x);\n",
      ASIN = "asin(x);\n",
      ACOS = "acos(x);\n",
      ATAN = "atan(x);\n",
      SINH = "sinh(x);\n",
      COSH = "cosh(x);\n",
      TANH = "tanh(x);\n",
      ASINH = "asinh(x);\n",
      ACOSH = "acosh(x);\n",
      ATANH = "atanh(x);\n",
      SQRT = "sqrt(x);\n",
      EXP = "exp(x);\n",
      POW = "pow(x,x);\n";

  std::pair<std::string, std::string>
      PRIM_ABS = std::make_pair("MathAbs_" + prim, FunMain(VarDecAss(prim, val) + ABS)),
      PRIM_SIN = std::make_pair("MathSin_" + prim, FunMain(VarDecAss(prim, val) + SIN)),
      PRIM_COS = std::make_pair("MathCos_" + prim, FunMain(VarDecAss(prim, val) + COS)),
      PRIM_TAN = std::make_pair("MathTan_" + prim, FunMain(VarDecAss(prim, val) + TAN)),
      PRIM_ASIN = std::make_pair("MathAsin_" + prim, FunMain(VarDecAss(prim, val) + ASIN)),
      PRIM_ACOS = std::make_pair("MathAcos_" + prim, FunMain(VarDecAss(prim, val) + ACOS)),
      PRIM_ATAN = std::make_pair("MathAtan_" + prim, FunMain(VarDecAss(prim, val) + ATAN)),
      PRIM_SINH = std::make_pair("MathSinh_" + prim, FunMain(VarDecAss(prim, val) + SINH)),
      PRIM_COSH = std::make_pair("MathCosh_" + prim, FunMain(VarDecAss(prim, val) + COSH)),
      PRIM_TANH = std::make_pair("MathTanh_" + prim, FunMain(VarDecAss(prim, val) + TANH)),
      PRIM_ASINH = std::make_pair("MathAsinh_" + prim, FunMain(VarDecAss(prim, val) + ASINH)),
      PRIM_ACOSH = std::make_pair("MathAcosh_" + prim, FunMain(VarDecAss(prim, alt_val) + ACOSH)),
      PRIM_ATANH = std::make_pair("MathAtanh_" + prim, FunMain(VarDecAss(prim, val) + ATANH)),
      PRIM_SQRT = std::make_pair("MathSqrt_" + prim, FunMain(VarDecAss(prim, val) + SQRT)),
      PRIM_EXP = std::make_pair("MathExp_" + prim, FunMain(VarDecAss(prim, val) + EXP)),
      PRIM_POW = std::make_pair("MathPow_" + prim, FunMain(VarDecAss(prim, val) + POW)),
      PRIM_RAND = std::make_pair("MathRand_" + prim, FunMain(Rand(val, alt_val)));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"MathAbs_" + prim,   "PrimPushVariable_" + prim},
                      {"MathSin_" + prim,   "PrimPushVariable_" + prim},
                      {"MathCos_" + prim,   "PrimPushVariable_" + prim},
                      {"MathTan_" + prim,   "PrimPushVariable_" + prim},
                      {"MathAsin_" + prim,  "PrimPushVariable_" + prim},
                      {"MathAcos_" + prim,  "PrimPushVariable_" + prim},
                      {"MathAtan_" + prim,  "PrimPushVariable_" + prim},
                      {"MathSinh_" + prim,  "PrimPushVariable_" + prim},
                      {"MathCosh_" + prim,  "PrimPushVariable_" + prim},
                      {"MathTanh_" + prim,  "PrimPushVariable_" + prim},
                      {"MathAsinh_" + prim, "PrimPushVariable_" + prim},
                      {"MathAcosh_" + prim, "PrimPushVariable_" + prim},
                      {"MathAtanh_" + prim, "PrimPushVariable_" + prim},
                      {"MathSqrt_" + prim,  "PrimPushVariable_" + prim},
                      {"MathExp_" + prim,   "PrimPushVariable_" + prim},
                      {"MathPow_" + prim,   "PrimPushVariable_" + prim},
                      {"MathRand_" + prim,  "Return"}
                  });

  std::vector<std::pair<std::string, std::string>> const
      etch_codes = {PRIM_ABS, PRIM_SIN, PRIM_COS, PRIM_TAN, PRIM_ASIN, PRIM_ACOS, PRIM_ATAN, PRIM_SINH, PRIM_COSH,
                    PRIM_TANH, PRIM_ASINH, PRIM_ACOSH, PRIM_ATANH, PRIM_SQRT, PRIM_EXP, PRIM_POW};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void ArrayBenchmarks(benchmark::State &state) {

  auto bm_ind = static_cast<u_int>(state.range(0));

  // Generate array lengths from benchmark parameters
  std::vector<u_int> array_len = LinearRangeVector<u_int>(max_array_len, n_array_lens);

  // Get the index of the array length corresponding to the benchmark range variable and the relevant etch code
  const u_int len_ind = (bm_ind - array_begin) / n_array_bms;
  const u_int etch_ind = (bm_ind - array_begin) % n_array_bms;

  const std::string 
      prim = "Int32",
      arr1 = "x",
      arr2 = "y",
      val = "1",
      ind = std::to_string(array_len[len_ind] - 1),
      length = std::to_string(array_len[len_ind]);
  
  // Operations
  static std::string
      COUNT = "x.count(;\n",
      REV = "x.reverse(;\n",
      POPBACK = "x.popBack(;\n",
      POPFRONT = "x.popFront(;\n";

  const std::pair<std::string, std::string>
      ARRAY_DEC = std::make_pair("DeclareArray_" + length, FunMain(ArrayDec(arr1, prim, length))),
      ARRAY_ASS = std::make_pair("AssignArray_" + length, FunMain(ArrayDec(arr1, prim, length) + ArrayAss(arr1, ind, val))),
      ARRAY_COUNT = std::make_pair("CountArray_" + length, FunMain(ArrayDec(arr1, prim, length) + COUNT)),
      ARRAY_APP = std::make_pair("AppendArray_" + length, FunMain(ArrayDec(arr1, prim, length) + ArrayAppend(arr1, val))),
      ARRAY_DEC_2 = std::make_pair("DeclareTwoArray_" + length, FunMain(ArrayDec(arr1, prim, length) + ArrayDec(arr2, prim, length))),
      ARRAY_EXT = std::make_pair("ExtendArray_" + length, FunMain(ArrayDec(arr1, prim, length) + ArrayDec(arr2, prim, length) + ArrayExtend(arr1, arr2))),
      ARRAY_POPBACK = std::make_pair("PopBackArray_" + length, FunMain(ArrayDec(arr1, prim, length) + POPBACK)),
      ARRAY_POPFRONT = std::make_pair("PopFrontArray_" + length, FunMain(ArrayDec(arr1, prim, length) + POPFRONT)),
      ARRAY_ERASE = std::make_pair("EraseArray_" + length, FunMain(ArrayDec(arr1, prim, length) + ArrayErase(arr1, ind))),
      ARRAY_REV = std::make_pair("ReverseArray_" + length, FunMain(ArrayDec(arr1, prim, length) + REV));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"DeclareArray_" + length,    "Return"},
                      {"AssignArray_" + length,     "DeclareArray_" + length},
                      {"CountArray_" + length,      "DeclareArray_" + length},
                      {"AppendArray_" + length,     "DeclareArray_" + length},
                      {"DeclareTwoArray_" + length, "Return"},
                      {"ExtendArray_" + length,     "DeclareTwoArray_" + length},
                      {"PopBackArray_" + length,    "DeclareArray_" + length},
                      {"PopFrontArray_" + length,   "DeclareArray_" + length},
                      {"EraseArray_" + length,      "DeclareArray_" + length},
                      {"ReverseArray_" + length,    "DeclareArray_" + length}
                  });

  std::vector<std::pair<std::string, std::string>> const
      etch_codes = {ARRAY_DEC, ARRAY_ASS, ARRAY_COUNT, ARRAY_APP, ARRAY_DEC_2, ARRAY_EXT, ARRAY_POPBACK, ARRAY_POPFRONT,
                    ARRAY_ERASE, ARRAY_REV};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void TensorBenchmarks(benchmark::State &state) {

  auto bm_ind = static_cast<u_int>(state.range(0));

  const u_int
      dim_size_ind = (bm_ind - tensor_begin) / n_tensor_bms, // index of all dim-size variations for a given benchmark
      dim (dim_size_ind / n_tensor_sizes + 2), // tensor dimension
      dim_begin ((dim - 2)*n_tensor_sizes), // index of first occurence of this tensor dimension
      size_ind (dim_size_ind - dim_begin), // index of tensor size vector
      etch_ind = (bm_ind - tensor_begin) % n_tensor_bms; // index of etch code snippet (benchmark type)

  // Generate tensor sizes from benchmark parameters
  const auto max_tensor_side = static_cast<u_int>(std::pow(static_cast<float>(max_tensor_size),1.0f / static_cast<float>(dim)));
  std::vector<u_int> tensor_sides = LinearRangeVector<u_int>(max_tensor_side, n_tensor_sizes);

  std::string const
      prim = "UInt64",
      tensor_shape = "shape",
      tensor = "tensor",
      val = "1";

  std::string const
      size = std::to_string(tensor_sides[size_ind]),
      size_u64 = size + "u64",
      dim_str = std::to_string(dim);

  // Operations
  static std::string
      SIZE = tensor + ".size();\n",
      FILL = tensor + ".fill(1.0fp64);\n",
      FILL_RAND = tensor + ".fillRandom();\n";

  const std::pair<std::string, std::string>
      TENSOR_DEC = std::make_pair("DeclareTensor_" + dim_str + "-" + size,
          FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim))),
      TENSOR_SIZE = std::make_pair("SizeTensor_" + dim_str + "-" + size,
          FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim) + SIZE)),
      TENSOR_FILL = std::make_pair("FillTensor_" + dim_str + "-" + size,
          FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim) + FILL)),
      TENSOR_FILL_RAND = std::make_pair("FillRandTensor_" + dim_str + "-" + size,
          FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim) + FILL_RAND));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"DeclareTensor_" + dim_str + "-" + size,  "Return"},
                      {"SizeTensor_" + dim_str + "-" + size,     "DeclareTensor_" + dim_str + "-" + size},
                      {"FillTensor_" + dim_str + "-" + size,     "DeclareTensor_" + dim_str + "-" + size},
                      {"FillRandTensor_" + dim_str + "-" + size, "DeclareTensor_" + dim_str + "-" + size}
                  });

  std::vector<std::pair<std::string, std::string>> const
      etch_codes = {TENSOR_DEC, TENSOR_SIZE, TENSOR_FILL, TENSOR_FILL_RAND};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void CryptoBenchmarks(benchmark::State &state) {

  auto bm_ind = static_cast<u_int>(state.range(0));

  // Generate the string lengths from benchmark parameters
  std::vector<u_int> str_len = LinearRangeVector<u_int>(max_str_len, n_str_lens);

  const u_int etch_ind = std::min<u_int>(bm_ind - crypto_begin, 3);
  const u_int len_ind = (bm_ind - (crypto_begin + 3)) % n_str_lens;

  std::string const length(std::to_string(str_len[len_ind]));

  // Operations
  static std::string
      DEC("var s = SHA256();\n"),
      RESET("s.reset();\n"),
      FINAL("s.final();\n");

  const std::pair<std::string, std::string>
      SHA256_DEC = std::make_pair("Sha256Declare", FunMain(DEC)),
      SHA256_RESET = std::make_pair("Sha256Reset", FunMain(DEC + RESET)),
      SHA256_FINAL = std::make_pair("Sha256Final", FunMain(DEC + FINAL)),
      SHA256_UPDATE = std::make_pair("Sha256Update_" + length, FunMain(DEC + Sha256Update(str_len[len_ind])));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"Sha256Declare",          "Return"},
                      {"Sha256Reset",            "Sha256Declare"},
                      {"Sha256Final",            "Sha256Declare"},
                      {"Sha256Update_" + length, "Sha256Declare"}
                  });

  std::vector<std::pair<std::string, std::string>> const
      etch_codes = {SHA256_DEC, SHA256_RESET, SHA256_FINAL, SHA256_UPDATE};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

bool RegisterBenchmarks() {
  BENCHMARK(BasicBenchmarks)->DenseRange(basic_begin, basic_end - 1, 1);
  if (run_object) { BENCHMARK(ObjectBenchmarks)->DenseRange(object_begin, object_end - 1, 1); }
  if (run_prim_ops) { BENCHMARK(PrimitiveOpBenchmarks)->DenseRange(prim_begin, prim_end - 1, 1); }
  if (run_math) { BENCHMARK(MathBenchmarks)->DenseRange(math_begin, math_end - 1, 1); }
  if (run_array) { BENCHMARK(ArrayBenchmarks)->DenseRange(array_begin, array_end - 1, 1); }
  if (run_tensor) { BENCHMARK(TensorBenchmarks)->DenseRange(tensor_begin, tensor_end - 1, 1); }
  if (run_crypto) { BENCHMARK(CryptoBenchmarks)->DenseRange(crypto_begin, crypto_end - 1, 1); }
  return true;
}

bool const vm_benchmark_success = RegisterBenchmarks();

} // namespace