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

#include <algorithm>
#include <cmath>
#include <fstream>

#include <vm_modules/core/byte_array_wrapper.hpp>
#include <vm_modules/crypto/sha256.hpp>
#include <vm_modules/math/bignumber.hpp>
#include <vm_modules/math/math.hpp>
#include <vm_modules/math/random.hpp>
#include <vm_modules/vm_factory.hpp>

#include "benchmark/benchmark.h"

#include "vm/compiler.hpp"
#include "vm/ir.hpp"
#include "vm/module.hpp"
#include "vm/opcodes.hpp"
#include "vm/vm.hpp"

using fetch::vm::VM;
using fetch::vm_modules::VMFactory;
using fetch::vm::Executable;
using fetch::vm::Variant;
using fetch::vm::Compiler;
using fetch::vm::IR;

using BenchmarkPair = std::pair<std::string, std::string>;

namespace {

/**These benchmarks are designed to profile the vm opcodes by generating and compiling Etch code
 * for each opcode or functionality to be profiled. The resulting times can then be compared to
 * appropriate baseline benchmarks to isolate the desired opcodes as much as possible.
 *
 * Benchmarks are best launched from the script "scripts/benchmark/opcode_timing.py"
 *
 *  To change the maximum size or number of sizes used for the parameterized benchmarks, change the
 *  constant under benchmark parameters below.
 *
 *  To add a new benchmark to an existing category:
 *  1. Increment the corresponding n_*_bms constant,
 *  2. Find the Benchmark function for the category (*Benchmarks(...))
 *  3. Add a BenchmarkPair specifying the name and corresponding Etch code to be executed
 *  4. Add this BenchmarkPair to the etch_codes vector
 *  5. Add the appropriate baseline benchmark to baseline_map
 *
 *  To add a new benchmark category:
 *  1. Create a new Benchmark function for the category using an existing one as a template
 *  2. Add BenchmarkPairs specifying the names and corresponding Etch codes to be executed
 *  3. Add each (benchmark, baseline) pair to baseline_map
 *  4. Define an indexing system depending on the parameters of the benchmark (see existing bms)
 *  5. Include each new benchmark in the etch_codes vector
 *  6. Add corresponding parameters and n_*_bms constants below
 *  7. Add any new required VM bindings to the top of EtchCodeBenchmarks(...)
 *  8. Register the new benchmarks in the function RegisterBenchmarks
 *  9. Update python script "scripts/benchmark/opcode_timing.py" as needed
 */

// Benchmark parameters (change as desired)
const uint32_t max_array_len = 16384, n_array_lens = 33, max_str_len = 16384, n_str_lens = 17,
               max_tensor_size = 531441, n_tensor_sizes = 17, n_dim_sizes = n_tensor_sizes * 3,
               max_crypto_len = 16384, n_crypto_lens = 17;

// Number of benchmarks in each category
const uint32_t n_basic_bms = 15, n_object_bms = 10, n_prim_bms = 25, n_math_bms = 16,
               n_array_bms = 10, n_tensor_bms = 5, n_crypto_bms = 6;

// Number of total (including int) and decimal (fixed or float) primitives
const uint32_t n_primitives = 13, n_dec_primitives = 5;

// Index benchmarks for interpretation by "scripts/benchmark/opcode_timing.py"
const uint32_t basic_begin = 0, basic_end = n_basic_bms;  // always run "Return"
const uint32_t object_begin = basic_end, object_end = object_begin + n_str_lens * n_object_bms;
const uint32_t prim_begin = object_end, prim_end = prim_begin + n_prim_bms * n_primitives;
const uint32_t math_begin = prim_end, math_end = math_begin + n_math_bms * n_dec_primitives;
const uint32_t array_begin = math_end, array_end = array_begin + n_array_bms * n_array_lens;
const uint32_t tensor_begin = array_end, tensor_end = tensor_begin + n_tensor_bms * n_dim_sizes;
const uint32_t crypto_begin = tensor_end,
               crypto_end   = crypto_begin + 1 + (n_crypto_bms - 1) * n_crypto_lens;

/**
 * Main benchmark function - compiles and runs Etch code snippets and saves opcodes to file
 *
 * @param state Google benchmark state variable
 * @param name Name of this particular benchmark
 * @param etch_code Etch code associated with benchmark
 * @param baseline_name Name of baseline benchmark (baseline time is subtracted to get net time)
 * @param Index of benchmark (used to encode several Etch-code benchmarks in each function)
 */
void EtchCodeBenchmark(benchmark::State &state, std::string const &benchmark_name,
                       std::string const &etch_code, std::string const &baseline_name,
                       uint32_t const bm_ind)
{
  auto     module = VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS);
  Compiler compiler(module.get());
  IR       ir;

  // Compile the source code
  std::vector<std::string> errors;
  fetch::vm::SourceFiles   files = {{"default.etch", etch_code}};
  if (!compiler.Compile(files, "default_ir", ir, errors))
  {
    std::cout << "Skipping benchmark (unable to compile): " << benchmark_name << std::endl;
    return;
  }

  // Generate an IR
  Executable executable;
  auto       vm = std::make_unique<VM>(module.get());
  if (!vm->GenerateExecutable(ir, "default_exe", executable, errors))
  {
    std::cout << "Skipping benchmark (unable to generate IR)" << std::endl;
    return;
  }

  // Benchmark iterations
  std::string error{};
  Variant     output{};
  for (auto _ : state)
  {
    vm->Execute(executable, "main", error, output);
  }

  auto function = executable.FindFunction("main");

  // Write opcode lists to file
  std::ofstream ofs("opcode_lists.csv", std::ios::app);
  ofs << bm_ind << "," << benchmark_name << "," << baseline_name << ",";
  for (auto &it : function->instructions)
  {
    ofs << it.opcode;
    if (it.opcode != fetch::vm::Opcodes::Return && it.opcode != fetch::vm::Opcodes::ReturnValue)
    {
      ofs << ",";
    }
  }
  ofs << std::endl;
  ofs.close();

  // The first time through, write all opcode information to file
  if (bm_ind == 0)
  {
    ofs.open("opcode_defs.csv", std::ios::out);
    uint32_t i = 0;
    for (auto &opcode : vm->GetOpcodeInfoArray())
    {
      ofs << i << "\t" << opcode.unique_name << "\t" << opcode.static_charge << std::endl;
      ++i;
    }
    ofs.close();
  }
}

// The following functions are all used to generate Etch code
std::string FunMain(std::string const &contents)
{
  return "function main()\n" + contents + "endfunction\n";
}

std::string FunMainRet(std::string const &contents, std::string const &return_type)
{
  return "function main() : " + return_type + "\n" + contents + "endfunction\n";
}

std::string FunUser(std::string const &contents)
{
  return "function user()\n" + contents + "endfunction\n";
}

std::string VarDec(std::string const &etchType)
{
  return "var x : " + etchType + ";\n";
}

std::string VarDecAss(std::string const &etchType, std::string const &value)
{
  return "var x : " + etchType + " = " + value + ";\n";
}

std::string IfThen(std::string const &condition, std::string const &consequent)
{
  return "if (" + condition + ")\n" + consequent + "endif\n";
}

std::string IfThenElse(std::string const &condition, std::string const &consequent,
                       std::string const &alternate)
{
  return "if (" + condition + ")\n" + consequent + "else\n" + alternate + "endif\n";
}

std::string For(std::string const &expression, std::string const &numIter)
{
  return "for (i in 0:" + numIter + ")\n" + expression + "endfor\n";
}

std::string Rand(std::string const &min, std::string const &max)
{
  return "rand(" + min + "," + max + ");\n";
}

std::string ArrayDec(std::string const &arr, std::string const &prim, std::string const &dim)
{
  return "var " + arr + " = Array<" + prim + ">(" + dim + ");\n";
}

std::string ArrayAss(std::string const &arr, std::string const &ind, std::string const &val)
{
  return arr + "[" + ind + "] = " + val + ";\n";
}

std::string ArrayAppend(std::string const &arr, std::string const &val)
{
  return arr + ".append(" + val + ");\n";
}

std::string ArrayExtend(std::string const &arr1, std::string const &arr2)
{
  return arr1 + ".extend(" + arr2 + ");\n";
}

std::string ArrayErase(std::string const &arr, std::string const &ind)
{
  return arr + ".erase(" + ind + ");\n";
}

std::string TensorDec(std::string const &tensor, std::string const &prim,
                      std::string const &tensor_shape, std::string const &tensor_size,
                      const uint32_t tensor_dim)
{
  auto tensor_dec = ArrayDec(tensor_shape, prim, std::to_string(tensor_dim));
  for (uint32_t i = 0; i != tensor_dim; ++i)
  {
    tensor_dec += ArrayAss(tensor_shape, std::to_string(i), tensor_size);
  }
  tensor_dec += "var " + tensor + " = Tensor(" + tensor_shape + ");\n";
  return tensor_dec;
}

std::string FromString(std::string const &tensor, std::string const &val, uint32_t tensor_size)
{
  std::string from_string = tensor + ".fromString(\"";
  for (uint32_t i = 0; i != tensor_size; ++i)
  {
    from_string += val;
    if (i < tensor_size - 1)
    {
      from_string += ",";
    }
  }
  from_string += "\");\n";
  return from_string;
}

std::string BufferDec(std::string const &buffer, std::string const &dim)
{
  return "var " + buffer + " = Buffer(" + dim + ");\n";
}

std::string Sha256UpdateStr(uint32_t str_len)
{
  std::string str(str_len, '0');
  return "s.update(\"" + str + "\");\n";
}

std::string Sha256UpdateBuf(std::string const &buffer)
{
  return "s.update(\"" + buffer + "\");\n";
}

/** Create a linear spaced range vector from zero to max of primitive type T
 *
 * @tparam max is the range maximum and last element in the vector
 * @param n_elem is the number of elements to include in the vector
 *
 * @return the range vector
 */
template <typename T>
std::vector<T> LinearRangeVector(T max, uint32_t n_elem)
{
  float      current_val = 1.0f;
  auto const step        = static_cast<float>(max) / static_cast<float>(n_elem - 1);

  std::vector<T> v(n_elem);
  for (auto it = v.begin(); it != v.end(); ++it)
  {
    *it = static_cast<T>(current_val);
    current_val += step;
  }
  v[0] = static_cast<T>(1);
  return v;
}

// The following functions correspond to benchmark categories, each calling EtchCodeBenchmark(...)
void BasicBenchmarks(benchmark::State &state)
{
  // Functions, variable declarations, and constants
  const static std::string FUN_CALL = "user();\n", BRK = "break;\n", CONT = "continue;\n",
                           ONE = "1", TRUE = "true", FALSE = "false", STRING = "String",
                           VAL_STRING = "\"x\"", EMPTY;

  // Boolean and type-neutral benchmark codes
  static BenchmarkPair RETURN("Return", FunMain(""));
  static BenchmarkPair PUSH_FALSE("PushFalse", FunMain(FALSE + ";\n"));
  static BenchmarkPair PUSH_TRUE("PushTrue", FunMain(TRUE + ";\n"));
  static BenchmarkPair JUMP_IF_FALSE("JumpIfFalse", FunMain(IfThen(FALSE, EMPTY)));
  static BenchmarkPair JUMP("Jump", FunMain(IfThenElse(FALSE, EMPTY, EMPTY)));
  static BenchmarkPair NOT("Not", FunMain("!true;\n"));
  static BenchmarkPair AND("And", FunMain("true && true;\n"));
  static BenchmarkPair OR("Or", FunMain("false || true ;\n"));
  static BenchmarkPair FOR_LOOP("ForLoop", FunMain(For(EMPTY, ONE)));
  static BenchmarkPair BREAK("Break", FunMain(For(BRK, ONE)));
  static BenchmarkPair CONTINUE("Continue", FunMain(For(CONT, ONE)));
  static BenchmarkPair DESTRUCT_BASE("DestructBase", FunMain(VarDec(STRING) + For(EMPTY, ONE)));
  static BenchmarkPair DESTRUCT("Destruct", FunMain(For(VarDec(STRING), ONE)));
  static BenchmarkPair FUNC("Function", FunMain(FUN_CALL) + FunUser(""));
  static BenchmarkPair VAR_DEC_STRING("VariableDeclareStr", FunMain(VarDec(STRING)));

  std::unordered_map<std::string, std::string> baseline_map({{"Return", "Return"},
                                                             {"PushFalse", "Return"},
                                                             {"PushTrue", "Return"},
                                                             {"JumpIfFalse", "Return"},
                                                             {"Jump", "JumpIfFalse"},
                                                             {"Not", "PushTrue"},
                                                             {"And", "PushTrue"},
                                                             {"Or", "PushTrue"},
                                                             {"ForLoop", "Return"},
                                                             {"Break", "ForLoop"},
                                                             {"Continue", "ForLoop"},
                                                             {"DestructBase", "ForLoop"},
                                                             {"Destruct", "DestructBase"},
                                                             {"Function", "Return"},
                                                             {"VariableDeclareStr", "Return"}});

  std::vector<BenchmarkPair> const etch_codes = {
      RETURN,   PUSH_FALSE, PUSH_TRUE, JUMP_IF_FALSE, JUMP,     NOT,  AND,           OR,
      FOR_LOOP, BREAK,      CONTINUE,  DESTRUCT_BASE, DESTRUCT, FUNC, VAR_DEC_STRING};

  auto bm_ind = static_cast<uint32_t>(state.range(0));

  uint32_t const etch_ind = bm_ind - basic_begin;

  if (etch_ind >= etch_codes.size())
  {
    std::cout << "Skipping benchmark (index out of range of benchmark category)" << std::endl;
    return;
  }

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baseline_map[etch_codes[etch_ind].first], bm_ind);
}

void ObjectBenchmarks(benchmark::State &state)
{
  auto bm_ind = static_cast<uint32_t>(state.range(0));

  // Generate the string lengths from benchmark parameters
  std::vector<uint32_t> str_len = LinearRangeVector<uint32_t>(max_str_len, n_str_lens);

  // Get the indices of the string length and etch code corresponding to the benchmark range
  // variable
  const uint32_t len_ind  = (bm_ind - object_begin) / n_object_bms;
  const uint32_t etch_ind = (bm_ind - object_begin) % n_object_bms;

  std::string const str("\"" + std::string(str_len[len_ind], '0') + "\"");

  std::string const length(std::to_string(str_len[len_ind]));

  const static std::string STRING = "String", PUSH = "x;\n", ADD = "x + x;\n", EQ = "x == x;\n",
                           NEQ = "x != x;\n", LT = "x < x;\n", GT = "x > x;\n", LTE = "x <= x;\n",
                           GTE = "x >= x;\n";

  const BenchmarkPair PUSH_STRING("PushString_" + length, FunMain(str + ";\n"));
  const BenchmarkPair VAR_DEC_ASS_STRING("VariableDeclareAssignString_" + length,
                                         FunMain(VarDecAss(STRING, str)));
  const BenchmarkPair PUSH_VAR_STRING("PushVariableString_" + length,
                                      FunMain(VarDecAss(STRING, str) + PUSH));
  const BenchmarkPair OBJ_EQ("ObjectEqualString_" + length, FunMain(VarDecAss(STRING, str) + EQ));
  const BenchmarkPair OBJ_NEQ("ObjectNotEqualString_" + length,
                              FunMain(VarDecAss(STRING, str) + NEQ));
  const BenchmarkPair OBJ_LT("ObjectLessThanString_" + length,
                             FunMain(VarDecAss(STRING, str) + LT));
  const BenchmarkPair OBJ_GT("ObjectGreaterThanString_" + length,
                             FunMain(VarDecAss(STRING, str) + GT));
  const BenchmarkPair OBJ_LTE("ObjectLessThanOrEqualString_" + length,
                              FunMain(VarDecAss(STRING, str) + LTE));
  const BenchmarkPair OBJ_GTE("ObjectGreaterThanOrEqualString_" + length,
                              FunMain(VarDecAss(STRING, str) + GTE));
  const BenchmarkPair OBJ_ADD("ObjectAddString_" + length, FunMain(VarDecAss(STRING, str) + ADD));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string> baseline_map(
      {{"PushString_" + length, "Return"},
       {"VariableDeclareAssignString_" + length, "Return"},
       {"PushVariableString_" + length, "VariableDeclareAssignString_" + length},
       {"ObjectEqualString_" + length, "PushVariableString_" + length},
       {"ObjectNotEqualString_" + length, "PushVariableString_" + length},
       {"ObjectLessThanString_" + length, "PushVariableString_" + length},
       {"ObjectLessThanOrEqualString_" + length, "PushVariableString_" + length},
       {"ObjectGreaterThanString_" + length, "PushVariableString_" + length},
       {"ObjectGreaterThanOrEqualString_" + length, "PushVariableString_" + length},
       {"ObjectAddString_" + length, "PushVariableString_" + length}});

  std::vector<BenchmarkPair> const etch_codes = {PUSH_STRING,     VAR_DEC_ASS_STRING,
                                                 PUSH_VAR_STRING, OBJ_EQ,
                                                 OBJ_NEQ,         OBJ_LT,
                                                 OBJ_GT,          OBJ_LTE,
                                                 OBJ_GTE,         OBJ_ADD};

  if (etch_ind >= etch_codes.size())
  {
    std::cout << "Skipping benchmark (index out of range of benchmark category)" << std::endl;
    return;
  }

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baseline_map[etch_codes[etch_ind].first], bm_ind);
}

void PrimitiveOpBenchmarks(benchmark::State &state)
{
  const static std::vector<std::string> primitives{
      "Int8",   "Int16",   "Int32",   "Int64",   "UInt8",   "UInt16",  "UInt32",
      "UInt64", "Float32", "Float64", "Fixed32", "Fixed64", "Fixed128"};

  const static std::vector<std::string> values{"1i8",     "1i16",    "1i32",    "1i64", "1u8",
                                               "1u16",    "1u32",    "1u64",    "0.5f", "0.5",
                                               "0.5fp32", "0.5fp64", "0.5fp128"};

  auto bm_ind = static_cast<uint32_t>(state.range(0));

  // Get the index of the primitive corresponding to the benchmark range variable and the relevant
  // etch code
  const uint32_t prim_ind = (bm_ind - prim_begin) / n_prim_bms;
  const uint32_t etch_ind = (bm_ind - prim_begin) % n_prim_bms;

  std::string const prim(primitives[prim_ind]);
  std::string const val(values[prim_ind]);

  // Operations
  const static std::string PUSH = "x;\n", POP = "x = x;\n", ADD = "x + x;\n", SUB = "x - x;\n",
                           MUL = "x * x;\n", DIV = "x / x;\n", MOD = "x % x;\n", NEG = "-x;\n",
                           EQ = "x == x;\n", NEQ = "x != x;\n", LT = "x < x;\n", GT = "x > x;\n",
                           LTE = "x <= x;\n", GTE = "x >= x;\n", PRE_INC = "++x;\n",
                           PRE_DEC = "--x;\n", POST_INC = "x++;\n", POST_DEC = "x--;\n",
                           INP_ADD = "x += x;\n", INP_SUB = "x -= x;\n", INP_MUL = "x *= x;\n",
                           INP_DIV = "x /= x;\n", INP_MOD = "x %= x;\n";

  const BenchmarkPair RET_VAL("PrimReturnValue_" + prim, FunMainRet("return " + val + ";\n", prim));
  const BenchmarkPair VAR_DEC("PrimVariableDeclare_" + prim, FunMain(VarDec(prim)));
  const BenchmarkPair VAR_DEC_ASS("PrimVariableDeclareAssign_" + prim,
                                  FunMain(VarDecAss(prim, val)));
  const BenchmarkPair PUSH_CONST("PrimPushConst_" + prim, FunMain(val + ";\n"));
  const BenchmarkPair PUSH_VAR("PrimPushVariable_" + prim, FunMain(VarDecAss(prim, val) + PUSH));
  const BenchmarkPair POP_TO_VAR("PrimPopToVariable_" + prim, FunMain(VarDecAss(prim, val) + POP));
  const BenchmarkPair PRIM_ADD("PrimAdd_" + prim, FunMain(VarDecAss(prim, val) + ADD));
  const BenchmarkPair PRIM_SUB("PrimSubtract_" + prim, FunMain(VarDecAss(prim, val) + SUB));
  const BenchmarkPair PRIM_MUL("PrimMultiply_" + prim, FunMain(VarDecAss(prim, val) + MUL));
  const BenchmarkPair PRIM_DIV("PrimDivide_" + prim, FunMain(VarDecAss(prim, val) + DIV));
  const BenchmarkPair PRIM_MOD("PrimModulo_" + prim, FunMain(VarDecAss(prim, val) + MOD));
  const BenchmarkPair PRIM_NEG("PrimNegate_" + prim, FunMain(VarDecAss(prim, val) + NEG));
  const BenchmarkPair PRIM_EQ("PrimEqual_" + prim, FunMain(VarDecAss(prim, val) + EQ));
  const BenchmarkPair PRIM_NEQ("PrimNotEqual_" + prim, FunMain(VarDecAss(prim, val) + NEQ));
  const BenchmarkPair PRIM_LT("PrimLessThan_" + prim, FunMain(VarDecAss(prim, val) + LT));
  const BenchmarkPair PRIM_GT("PrimGreaterThan_" + prim, FunMain(VarDecAss(prim, val) + GT));
  const BenchmarkPair PRIM_LTE("PrimLessThanOrEqual_" + prim, FunMain(VarDecAss(prim, val) + LTE));
  const BenchmarkPair PRIM_GTE("PrimGreaterThanOrEqual_" + prim,
                               FunMain(VarDecAss(prim, val) + GTE));
  const BenchmarkPair PRIM_PRE_INC("PrimVariablePrefixInc_" + prim,
                                   FunMain(VarDecAss(prim, val) + PRE_INC));
  const BenchmarkPair PRIM_PRE_DEC("PrimVariablePrefixDec_" + prim,
                                   FunMain(VarDecAss(prim, val) + PRE_DEC));
  const BenchmarkPair PRIM_POST_INC("PrimVariablePostfixInc_" + prim,
                                    FunMain(VarDecAss(prim, val) + POST_INC));
  const BenchmarkPair PRIM_POST_DEC("PrimVariablePostfixDec_" + prim,
                                    FunMain(VarDecAss(prim, val) + POST_DEC));
  const BenchmarkPair VAR_PRIM_INP_ADD("PrimVariableInplaceAdd_" + prim,
                                       FunMain(VarDecAss(prim, val) + INP_ADD));
  const BenchmarkPair VAR_PRIM_INP_SUB("PrimVariableInplaceSubtract_" + prim,
                                       FunMain(VarDecAss(prim, val) + INP_SUB));
  const BenchmarkPair VAR_PRIM_INP_MUL("PrimVariableInplaceMultiply_" + prim,
                                       FunMain(VarDecAss(prim, val) + INP_MUL));
  const BenchmarkPair VAR_PRIM_INP_DIV("PrimVariableInplaceDivide_" + prim,
                                       FunMain(VarDecAss(prim, val) + INP_DIV));
  const BenchmarkPair VAR_PRIM_INP_MOD("PrimVariableInplaceModulo_" + prim,
                                       FunMain(VarDecAss(prim, val) + INP_MOD));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string> baseline_map(
      {{"PrimReturnValue_" + prim, "Return"},
       {"PrimVariableDeclare_" + prim, "Return"},
       {"PrimVariableDeclareAssign_" + prim, "Return"},
       {"PrimPushConst_" + prim, "Return"},
       {"PrimPushVariable_" + prim, "PrimPushConst_" + prim},
       {"PrimPopToVariable_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimAdd_" + prim, "PrimPushVariable_" + prim},
       {"PrimSubtract_" + prim, "PrimPushVariable_" + prim},
       {"PrimMultiply_" + prim, "PrimPushVariable_" + prim},
       {"PrimDivide_" + prim, "PrimPushVariable_" + prim},
       {"PrimModulo_" + prim, "PrimPushVariable_" + prim},
       {"PrimNegate_" + prim, "PrimPushVariable_" + prim},
       {"PrimEqual_" + prim, "PrimPushVariable_" + prim},
       {"PrimNotEqual_" + prim, "PrimPushVariable_" + prim},
       {"PrimLessThan_" + prim, "PrimPushVariable_" + prim},
       {"PrimGreaterThan_" + prim, "PrimPushVariable_" + prim},
       {"PrimLessThanOrEqual_" + prim, "PrimPushVariable_" + prim},
       {"PrimGreaterThanOrEqual_" + prim, "PrimPushVariable_" + prim},
       {"PrimVariablePrefixInc_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariablePrefixDec_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariablePostfixInc_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariablePostfixDec_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariableInplaceAdd_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariableInplaceSubtract_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariableInplaceMultiply_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariableInplaceDivide_" + prim, "PrimVariableDeclareAssign_" + prim},
       {"PrimVariableInplaceModulo_" + prim, "PrimVariableDeclareAssign_" + prim}});

  std::vector<BenchmarkPair> const etch_codes = {
      RET_VAL,         VAR_DEC,          VAR_DEC_ASS,      PUSH_CONST,       PUSH_VAR,
      POP_TO_VAR,      PRIM_ADD,         PRIM_SUB,         PRIM_MUL,         PRIM_DIV,
      PRIM_NEG,        PRIM_EQ,          PRIM_NEQ,         PRIM_LT,          PRIM_GT,
      PRIM_LTE,        PRIM_GTE,         PRIM_PRE_INC,     PRIM_PRE_DEC,     PRIM_POST_INC,
      PRIM_POST_DEC,   VAR_PRIM_INP_ADD, VAR_PRIM_INP_SUB, VAR_PRIM_INP_MUL, VAR_PRIM_INP_DIV,
      VAR_PRIM_INP_MOD};

  if (etch_ind >= etch_codes.size())
  {
    std::cout << "Skipping benchmark (index out of range of benchmark category)" << std::endl;
    return;
  }

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baseline_map[etch_codes[etch_ind].first], bm_ind);
}

void MathBenchmarks(benchmark::State &state)
{
  static std::vector<std::string> primitives{"Float32", "Float64", "Fixed32", "Fixed64",
                                             "Fixed128"};
  static std::vector<std::string> values{"0.5f", "0.5", "0.5fp32", "0.5fp64", "0.5fp128"};
  static std::vector<std::string> alt_values{"1.5f", "1.5", "1.5fp32", "1.5fp64", "1.5fp128"};

  auto bm_ind = static_cast<uint32_t>(state.range(0));

  // Get the index of the math function corresponding to the benchmark range variable and the
  // relevant etch code
  const uint32_t prim_ind = (bm_ind - math_begin) / n_math_bms;
  const uint32_t etch_ind = (bm_ind - math_begin) % n_math_bms;

  const std::string prim = primitives[prim_ind], val = values[prim_ind],
                    alt_val = alt_values[prim_ind];

  // Operations
  const static std::string PUSH = "x;\n", ABS = "abs(x);\n", SIN = "sin(x);\n", COS = "cos(x);\n",
                           TAN = "tan(x);\n", ASIN = "asin(x);\n", ACOS = "acos(x);\n",
                           ATAN = "atan(x);\n", SINH = "sinh(x);\n", COSH = "cosh(x);\n",
                           TANH = "tanh(x);\n", ASINH = "asinh(x);\n", ACOSH = "acosh(x);\n",
                           ATANH = "atanh(x);\n", SQRT = "sqrt(x);\n", EXP = "exp(x);\n",
                           POW = "pow(x,x);\n";

  const BenchmarkPair PRIM_ABS("MathAbs_" + prim, FunMain(VarDecAss(prim, val) + ABS)),
      PRIM_SIN("MathSin_" + prim, FunMain(VarDecAss(prim, val) + SIN)),
      PRIM_COS("MathCos_" + prim, FunMain(VarDecAss(prim, val) + COS)),
      PRIM_TAN("MathTan_" + prim, FunMain(VarDecAss(prim, val) + TAN)),
      PRIM_ASIN("MathAsin_" + prim, FunMain(VarDecAss(prim, val) + ASIN)),
      PRIM_ACOS("MathAcos_" + prim, FunMain(VarDecAss(prim, val) + ACOS)),
      PRIM_ATAN("MathAtan_" + prim, FunMain(VarDecAss(prim, val) + ATAN)),
      PRIM_SINH("MathSinh_" + prim, FunMain(VarDecAss(prim, val) + SINH)),
      PRIM_COSH("MathCosh_" + prim, FunMain(VarDecAss(prim, val) + COSH)),
      PRIM_TANH("MathTanh_" + prim, FunMain(VarDecAss(prim, val) + TANH)),
      PRIM_ASINH("MathAsinh_" + prim, FunMain(VarDecAss(prim, val) + ASINH)),
      PRIM_ACOSH("MathAcosh_" + prim, FunMain(VarDecAss(prim, alt_val) + ACOSH)),
      PRIM_ATANH("MathAtanh_" + prim, FunMain(VarDecAss(prim, val) + ATANH)),
      PRIM_SQRT("MathSqrt_" + prim, FunMain(VarDecAss(prim, val) + SQRT)),
      PRIM_EXP("MathExp_" + prim, FunMain(VarDecAss(prim, val) + EXP)),
      PRIM_POW("MathPow_" + prim, FunMain(VarDecAss(prim, val) + POW)),
      PRIM_RAND("MathRand_" + prim, FunMain(Rand(val, alt_val)));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string> baseline_map(
      {{"MathAbs_" + prim, "PrimPushVariable_" + prim},
       {"MathSin_" + prim, "PrimPushVariable_" + prim},
       {"MathCos_" + prim, "PrimPushVariable_" + prim},
       {"MathTan_" + prim, "PrimPushVariable_" + prim},
       {"MathAsin_" + prim, "PrimPushVariable_" + prim},
       {"MathAcos_" + prim, "PrimPushVariable_" + prim},
       {"MathAtan_" + prim, "PrimPushVariable_" + prim},
       {"MathSinh_" + prim, "PrimPushVariable_" + prim},
       {"MathCosh_" + prim, "PrimPushVariable_" + prim},
       {"MathTanh_" + prim, "PrimPushVariable_" + prim},
       {"MathAsinh_" + prim, "PrimPushVariable_" + prim},
       {"MathAcosh_" + prim, "PrimPushVariable_" + prim},
       {"MathAtanh_" + prim, "PrimPushVariable_" + prim},
       {"MathSqrt_" + prim, "PrimPushVariable_" + prim},
       {"MathExp_" + prim, "PrimPushVariable_" + prim},
       {"MathPow_" + prim, "PrimPushVariable_" + prim},
       {"MathRand_" + prim, "Return"}});

  std::vector<BenchmarkPair> const etch_codes = {
      PRIM_ABS,  PRIM_SIN,  PRIM_COS,   PRIM_TAN,   PRIM_ASIN,  PRIM_ACOS, PRIM_ATAN, PRIM_SINH,
      PRIM_COSH, PRIM_TANH, PRIM_ASINH, PRIM_ACOSH, PRIM_ATANH, PRIM_SQRT, PRIM_EXP,  PRIM_POW};

  if (etch_ind >= etch_codes.size())
  {
    std::cout << "Skipping benchmark (index out of range of benchmark category)" << std::endl;
    return;
  }

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baseline_map[etch_codes[etch_ind].first], bm_ind);
}

void ArrayBenchmarks(benchmark::State &state)
{
  auto bm_ind = static_cast<uint32_t>(state.range(0));

  // Generate array lengths from benchmark parameters
  std::vector<uint32_t> array_len = LinearRangeVector<uint32_t>(max_array_len, n_array_lens);

  // Length and etch-code indices corresponding to benchmark range variable
  const uint32_t len_ind  = (bm_ind - array_begin) / n_array_bms;
  const uint32_t etch_ind = (bm_ind - array_begin) % n_array_bms;

  const std::string prim = "Int32", arr1 = "x", arr2 = "y", val = "1",
                    ind = std::to_string(array_len[len_ind] - 1),
                    len = std::to_string(array_len[len_ind]);

  // Operations
  const static std::string COUNT = "x.count();\n", REV = "x.reverse();\n",
                           POPBACK = "x.popBack();\n", POPFRONT = "x.popFront();\n";

  const BenchmarkPair ARRAY_DEC("DeclareArray_" + len, FunMain(ArrayDec(arr1, prim, len)));
  const BenchmarkPair ARRAY_ASS("AssignArray_" + len,
                                FunMain(ArrayDec(arr1, prim, len) + ArrayAss(arr1, ind, val)));
  const BenchmarkPair ARRAY_COUNT("CountArray_" + len, FunMain(ArrayDec(arr1, prim, len) + COUNT));
  const BenchmarkPair ARRAY_APP("AppendArray_" + len,
                                FunMain(ArrayDec(arr1, prim, len) + ArrayAppend(arr1, val)));
  const BenchmarkPair ARRAY_DEC_2("DeclareTwoArray_" + len,
                                  FunMain(ArrayDec(arr1, prim, len) + ArrayDec(arr2, prim, len)));
  const BenchmarkPair ARRAY_EXT(
      "ExtendArray_" + len,
      FunMain(ArrayDec(arr1, prim, len) + ArrayDec(arr2, prim, len) + ArrayExtend(arr1, arr2)));
  const BenchmarkPair ARRAY_POPBACK("PopBackArray_" + len,
                                    FunMain(ArrayDec(arr1, prim, len) + POPBACK));
  const BenchmarkPair ARRAY_POPFRONT("PopFrontArray_" + len,
                                     FunMain(ArrayDec(arr1, prim, len) + POPFRONT));
  const BenchmarkPair ARRAY_ERASE("EraseArray_" + len,
                                  FunMain(ArrayDec(arr1, prim, len) + ArrayErase(arr1, ind)));
  const BenchmarkPair ARRAY_REV("ReverseArray_" + len, FunMain(ArrayDec(arr1, prim, len) + REV));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string> baseline_map(
      {{"DeclareArray_" + len, "Return"},
       {"AssignArray_" + len, "DeclareArray_" + len},
       {"CountArray_" + len, "DeclareArray_" + len},
       {"AppendArray_" + len, "DeclareArray_" + len},
       {"DeclareTwoArray_" + len, "Return"},
       {"ExtendArray_" + len, "DeclareTwoArray_" + len},
       {"PopBackArray_" + len, "DeclareArray_" + len},
       {"PopFrontArray_" + len, "DeclareArray_" + len},
       {"EraseArray_" + len, "DeclareArray_" + len},
       {"ReverseArray_" + len, "DeclareArray_" + len}});

  std::vector<BenchmarkPair> const etch_codes = {
      ARRAY_DEC, ARRAY_ASS,     ARRAY_COUNT,    ARRAY_APP,   ARRAY_DEC_2,
      ARRAY_EXT, ARRAY_POPBACK, ARRAY_POPFRONT, ARRAY_ERASE, ARRAY_REV};

  if (etch_ind >= etch_codes.size())
  {
    std::cout << "Skipping benchmark (index out of range of benchmark category)" << std::endl;
    return;
  }

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baseline_map[etch_codes[etch_ind].first], bm_ind);
}

void TensorBenchmarks(benchmark::State &state)
{
  auto bm_ind = static_cast<uint32_t>(state.range(0));

  // Indexing systems to cycle through each combination of (benchmark, tensor dimension, size)
  const uint32_t dim_size_ind = (bm_ind - tensor_begin) / n_tensor_bms,
                 dim(dim_size_ind / n_tensor_sizes + 2), dim_begin((dim - 2) * n_tensor_sizes),
                 size_ind(dim_size_ind - dim_begin),
                 etch_ind = (bm_ind - tensor_begin) % n_tensor_bms;

  // Generate tensor sizes from benchmark parameters
  const auto max_tensor_side = static_cast<uint32_t>(
      std::pow(static_cast<float>(max_tensor_size), 1.0f / static_cast<float>(dim)));
  std::vector<uint32_t> tensor_sides = LinearRangeVector<uint32_t>(max_tensor_side, n_tensor_sizes);
  const auto            n_elem       = static_cast<uint32_t>(
      std::pow(static_cast<float>(tensor_sides[size_ind]), static_cast<float>(dim)));

  // Etch primitive types, values, and variable names
  const static std::string prim = "UInt64", tensor_shape = "shape", tensor = "tensor", val = "1";
  const std::string        size = std::to_string(tensor_sides[size_ind]), size_u64 = size + "u64",
                    dim_str = std::to_string(dim);

  // Operations
  static std::string SIZE = tensor + ".size();\n", FILL = tensor + ".fill(1.0fp64);\n",
                     FILL_RAND = tensor + ".fillRandom();\n";

  const BenchmarkPair TENSOR_DEC("DeclareTensor_" + dim_str + "-" + size,
                                 FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim)));
  const BenchmarkPair TENSOR_SIZE(
      "SizeTensor_" + dim_str + "-" + size,
      FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim) + SIZE));
  const BenchmarkPair TENSOR_FILL(
      "FillTensor_" + dim_str + "-" + size,
      FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim) + FILL));
  const BenchmarkPair TENSOR_FILL_RAND(
      "FillRandTensor_" + dim_str + "-" + size,
      FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim) + FILL_RAND));
  const BenchmarkPair TENSOR_FROM_STR("FromStrTensor_" + dim_str + "-" + size,
                                      FunMain(TensorDec(tensor, prim, tensor_shape, size_u64, dim) +
                                              FromString(tensor, val, n_elem)));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string> baseline_map(
      {{"DeclareTensor_" + dim_str + "-" + size, "Return"},
       {"SizeTensor_" + dim_str + "-" + size, "DeclareTensor_" + dim_str + "-" + size},
       {"FillTensor_" + dim_str + "-" + size, "DeclareTensor_" + dim_str + "-" + size},
       {"FillRandTensor_" + dim_str + "-" + size, "DeclareTensor_" + dim_str + "-" + size},
       {"FromStrTensor_" + dim_str + "-" + size, "DeclareTensor_" + dim_str + "-" + size}});

  std::vector<BenchmarkPair> const etch_codes = {TENSOR_DEC, TENSOR_SIZE, TENSOR_FILL,
                                                 TENSOR_FILL_RAND, TENSOR_FROM_STR};

  if (etch_ind >= etch_codes.size())
  {
    std::cout << "Skipping benchmark (index out of range of benchmark category)" << std::endl;
    return;
  }

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baseline_map[etch_codes[etch_ind].first], bm_ind);
}

void CryptoBenchmarks(benchmark::State &state)
{
  auto bm_ind = static_cast<uint32_t>(state.range(0));

  // Generate the string lengths from benchmark parameters
  std::vector<uint32_t> lengths = LinearRangeVector<uint32_t>(max_crypto_len, n_crypto_lens);

  // Length and etch-code indices corresponding to benchmark range variable
  uint32_t len_ind  = 0;
  uint32_t etch_ind = 0;

  // The first SHA256 benchmark is type independent, but the rest depend on length
  if ((bm_ind - crypto_begin) >= 1)
  {
    len_ind  = (bm_ind - (crypto_begin + 1)) / (n_crypto_bms - 1);
    etch_ind = 1 + (bm_ind - (crypto_begin + 1)) % (n_crypto_bms - 1);
  }

  std::string const length = std::to_string(lengths[len_ind]);
  std::string const buffer = "buffer";

  // Operations
  const static std::string DEC("var s = SHA256();\n"), RESET("s.reset();\n"), FINAL("s.final();\n");

  const BenchmarkPair SHA256_DEC("Sha256Declare", FunMain(DEC));
  const BenchmarkPair SHA256_RESET("Sha256Reset_" + length, FunMain(DEC + RESET));
  const BenchmarkPair SHA256_FINAL("Sha256Final_" + length, FunMain(DEC + FINAL));
  const BenchmarkPair SHA256_BUF_DEC("Sha256DeclareBuf_" + length,
                                     FunMain(BufferDec(buffer, length)));
  const BenchmarkPair SHA256_UPDATE_STR("Sha256UpdateStr_" + length,
                                        FunMain(DEC + Sha256UpdateStr(lengths[len_ind])));
  const BenchmarkPair SHA256_UPDATE_BUF(
      "Sha256UpdateBuf_" + length,
      FunMain(DEC + BufferDec(buffer, length) + Sha256UpdateBuf(buffer)));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string> baseline_map(
      {{"Sha256Declare", "Return"},
       {"Sha256Reset_" + length, "Sha256Declare"},
       {"Sha256Final_" + length, "Sha256Declare"},
       {"Sha256DeclareBuf_" + length, "Sha256Declare"},
       {"Sha256UpdateStr_" + length, "Sha256Declare"},
       {"Sha256UpdateBuf_" + length, "Sha256DeclareBuf_" + length}});

  std::vector<BenchmarkPair> const etch_codes = {
      SHA256_DEC, SHA256_RESET, SHA256_FINAL, SHA256_BUF_DEC, SHA256_UPDATE_STR, SHA256_UPDATE_BUF};

  if (etch_ind >= etch_codes.size())
  {
    std::cout << "Skipping benchmark (index out of range of benchmark category)" << std::endl;
    return;
  }

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baseline_map[etch_codes[etch_ind].first], bm_ind);
}

bool RegisterBenchmarks()
{
  BENCHMARK(BasicBenchmarks)->DenseRange(basic_begin, basic_end - 1, 1);
  BENCHMARK(ObjectBenchmarks)->DenseRange(object_begin, object_end - 1, 1);
  BENCHMARK(PrimitiveOpBenchmarks)->DenseRange(prim_begin, prim_end - 1, 1);
  BENCHMARK(MathBenchmarks)->DenseRange(math_begin, math_end - 1, 1);
  BENCHMARK(ArrayBenchmarks)->DenseRange(array_begin, array_end - 1, 1);
  BENCHMARK(TensorBenchmarks)->DenseRange(tensor_begin, tensor_end - 1, 1);
  BENCHMARK(CryptoBenchmarks)->DenseRange(crypto_begin, crypto_end - 1, 1);
  return true;
}

bool const vm_benchmark_success = RegisterBenchmarks();

}  // namespace
