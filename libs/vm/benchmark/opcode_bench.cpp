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

u_int const n_basic_bms = 13, n_object_bms = 11, n_prim_bms = 25, n_math_bms = 16, n_crypto_bms = 4;
u_int const n_primitives = 11, n_dec_primitives = 4;

u_int const
    basic_begin = 0,
    basic_end = n_basic_bms,
    object_begin = basic_end,
    object_end = object_begin + n_object_bms,
    prim_begin = object_end,
    prim_end = prim_begin + n_prim_bms*n_primitives,
    math_begin = prim_end,
    math_end = math_begin + n_math_bms*n_dec_primitives,
    crypto_begin = math_end,
    crypto_end = crypto_begin + n_crypto_bms;

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

  // compile the source code
  std::vector<std::string> errors;
  fetch::vm::SourceFiles files = {{"default.etch", etch_code}};
  if (!compiler.Compile(files, "default_ir", ir, errors)) {
    std::cout << "Skipping benchmark (unable to compile): " << benchmark_name << std::endl;
    return;
  }

  // generate an IR
  Executable executable;
  VM vm{&module};
  if (!vm.GenerateExecutable(ir, "default_exe", executable, errors)) {
    std::cout << "Skipping benchmark (unable to generate IR)" << std::endl;
    return;
  }

  // benchmark iterations
  std::string error{};
  Variant output{};
  for (auto _ : state) {
    vm.Execute(executable, "main", error, output);
  }

  auto function = executable.FindFunction("main");

  // write opcode lists to file
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

std::string Sha256Update(u_int str_len) {
  std::string str(str_len, '0');
  return "s.update(\"" + str + "\");\n";
}

void BasicBenchmarks(benchmark::State &state) {

  // Functions, variable declarations, and constants
  static std::string
      FUN_CALL = "user();\n",
      BRK("break;\n"),
      CONT("continue;\n"),
      ONE("1"),
      TRUE("true"),
      FALSE = "false",
      STRING("String"),
      VAL_STRING("\"x\""),
      EMPTY;

  // Boolean and type-neutral benchmark codes
  static std::pair<std::string, std::string>
      RETURN("Return", FunMain("")),
      PUSH_NULL("PushNull", FunMain("null;\n")),
      PUSH_FALSE("PushFalse", FunMain(FALSE + ";\n")),
      PUSH_TRUE("PushTrue", FunMain(TRUE + ";\n")),
      JUMP_IF_FALSE("JumpIfFalse", FunMain(IfThen(FALSE, EMPTY))),
      JUMP("Jump", FunMain(IfThenElse(FALSE, EMPTY, EMPTY))),
      NOT("Not", FunMain("!true;\n")),
      FOR_LOOP("ForLoop", FunMain(For(EMPTY, ONE))),
      BREAK("Break", FunMain(For(BRK, ONE))),
      CONTINUE("Continue", FunMain(For(CONT, ONE))),
      DESTRUCT_BASE("DestructBase", FunMain(VarDec(STRING) + For(EMPTY, ONE))),
      DESTRUCT("Destruct", FunMain(For(VarDec(STRING), ONE))),
      FUNC("Function", FunMain(FUN_CALL) + FunUser(""));

  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"Return",       "Return"},
                      {"PushNull",     "Return"},
                      {"PushFalse",    "Return"},
                      {"PushTrue",     "Return"},
                      {"JumpIfFalse",  "Return"},
                      {"Jump",         "JumpIfFalse"},
                      {"Not",          "PushTrue"},
                      {"ForLoop",      "Return"},
                      {"Break",        "ForLoop"},
                      {"Continue",     "ForLoop"},
                      {"DestructBase", "ForLoop"},
                      {"Destruct",     "DestructBase"},
                      {"Function",     "Return"}
                  });

  std::vector<std::pair<std::string, std::string>>
      etch_codes = {RETURN, PUSH_NULL, PUSH_FALSE, PUSH_TRUE, JUMP_IF_FALSE, JUMP, NOT, FOR_LOOP, BREAK, CONTINUE,
                    DESTRUCT_BASE, DESTRUCT, FUNC};

  auto bm_ind = static_cast<u_int>(state.range(0));

  u_int etch_ind = bm_ind - basic_begin;

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second, baselineMap[etch_codes[etch_ind].first],
                    bm_ind);
}

void ObjectBenchmarks(benchmark::State &state) {

  static std::string
      STRING("String"),
      VAL_STRING("\"x\""),
      PUSH("x;\n"),
      ADD("x + x;\n"),
      EQ("x == x;\n"),
      NEQ("x != x;\n"),
      LT("x < x;\n"),
      GT("x > x;\n"),
      LTE("x <= x;\n"),
      GTE("x >= x;\n");

  static std::pair<std::string, std::string>
      PUSH_STRING("PushString", FunMain(VAL_STRING + ";\n")),
      VAR_DEC_STRING("VariableDeclareString", FunMain(VarDec(STRING))),
      VAR_DEC_ASS_STRING("VariableDeclareAssignString", FunMain(VarDecAss(STRING, VAL_STRING))),
      PUSH_VAR_STRING("PushVariableString", FunMain(VarDecAss(STRING, VAL_STRING) + PUSH)),
      OBJ_EQ("ObjectEqualString", FunMain(VarDecAss(STRING, VAL_STRING) + EQ)),
      OBJ_NEQ("ObjectNotEqualString", FunMain(VarDecAss(STRING, VAL_STRING) + NEQ)),
      OBJ_LT("ObjectLessThanString", FunMain(VarDecAss(STRING, VAL_STRING) + LT)),
      OBJ_GT("ObjectGreaterThanString", FunMain(VarDecAss(STRING, VAL_STRING) + GT)),
      OBJ_LTE("ObjectLessThanOrEqualString", FunMain(VarDecAss(STRING, VAL_STRING) + LTE)),
      OBJ_GTE("ObjectGreaterThanOrEqualString", FunMain(VarDecAss(STRING, VAL_STRING) + GTE)),
      OBJ_ADD("ObjectAddString", FunMain(VarDecAss(STRING, VAL_STRING) + ADD));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"PushString",                     "Return"},
                      {"VariableDeclareString",          "Return"},
                      {"VariableDeclareAssignString",    "Return"},
                      {"PushVariableString",             "VariableDeclareAssignString"},
                      {"ObjectEqualString",              "PushVariableString"},
                      {"ObjectNotEqualString",           "PushVariableString"},
                      {"ObjectLessThanString",           "PushVariableString"},
                      {"ObjectLessThanOrEqualString",    "PushVariableString"},
                      {"ObjectGreaterThanString",        "PushVariableString"},
                      {"ObjectGreaterThanOrEqualString", "PushVariableString"},
                      {"ObjectAddString",                "PushVariableString"}
                  });

  std::vector<std::pair<std::string, std::string>>
      etch_codes = {PUSH_STRING, VAR_DEC_STRING, VAR_DEC_ASS_STRING, PUSH_VAR_STRING, OBJ_EQ, OBJ_NEQ, OBJ_LT, OBJ_GT,
                    OBJ_LTE, OBJ_GTE, OBJ_ADD};

  auto bm_ind = static_cast<u_int>(state.range(0));

  u_int etch_ind = bm_ind - object_begin;

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second, baselineMap[etch_codes[etch_ind].first],
                    bm_ind);
}

void PrimitiveOpBenchmarks(benchmark::State &state) {

  static std::vector<std::string> primitives{"Int16", "Int32", "Int64", "UInt8", "UInt16", "UInt32", "UInt64",
                                             "Float32", "Float64", "Fixed32", "Fixed64"};

  static std::vector<std::string> values{"1i16", "1i32", "1i64", "1u8", "1u16", "1u32", "1u64",
                                         "0.5f", "0.5", "0.5fp32", "0.5fp64"};

  auto bm_ind = static_cast<u_int>(state.range(0));

  // Get the index of the primitive corresponding to the benchmark range variable
  u_int prim_ind = (bm_ind - prim_begin) / n_prim_bms;
  u_int etch_ind = (bm_ind - prim_begin) % n_prim_bms;

  std::string prim(primitives[prim_ind]);
  std::string val(values[prim_ind]);

  // Operations
  static std::string
      PUSH("x;\n"),
      POP("x = x;\n"),
      ADD("x + x;\n"),
      SUB("x - x;\n"),
      MUL("x * x;\n"),
      DIV("x / x;\n"),
      MOD("x % x;\n"),
      NEG("-x;\n"),
      EQ("x == x;\n"),
      NEQ("x != x;\n"),
      LT("x < x;\n"),
      GT("x > x;\n"),
      LTE("x <= x;\n"),
      GTE("x >= x;\n"),
      PRE_INC("++x;\n"),
      PRE_DEC("--x;\n"),
      POST_INC("x++;\n"),
      POST_DEC("x--;\n"),
      INP_ADD("x += x;\n"),
      INP_SUB("x -= x;\n"),
      INP_MUL("x *= x;\n"),
      INP_DIV("x /= x;\n"),
      INP_MOD("x %= x;\n");

  std::pair<std::string, std::string>
      RET_VAL = std::make_pair("ReturnValue" + prim, FunMainRet("return " + val + ";\n", prim)),
      VAR_DEC = std::make_pair("VariableDeclare" + prim, FunMain(VarDec(prim))),
      VAR_DEC_ASS = std::make_pair("VariableDeclareAssign" + prim, FunMain(VarDecAss(prim, val))),
      PUSH_CONST = std::make_pair("PushConst" + prim, FunMain(val + ";\n")),
      PUSH_VAR = std::make_pair("PushVariable" + prim, FunMain(VarDecAss(prim, val) + PUSH)),
      POP_TO_VAR = std::make_pair("PopToVariable" + prim, FunMain(VarDecAss(prim, val) + POP)),
      PRIM_ADD = std::make_pair("PrimitiveAdd" + prim, FunMain(VarDecAss(prim, val) + ADD)),
      PRIM_SUB = std::make_pair("PrimitiveSubtract" + prim, FunMain(VarDecAss(prim, val) + SUB)),
      PRIM_MUL = std::make_pair("PrimitiveMultiply" + prim, FunMain(VarDecAss(prim, val) + MUL)),
      PRIM_DIV = std::make_pair("PrimitiveDivide" + prim, FunMain(VarDecAss(prim, val) + DIV)),
      PRIM_MOD = std::make_pair("PrimitiveModulo" + prim, FunMain(VarDecAss(prim, val) + MOD)),
      PRIM_NEG = std::make_pair("PrimitiveNegate" + prim, FunMain(VarDecAss(prim, val) + NEG)),
      PRIM_EQ = std::make_pair("PrimitiveEqual" + prim, FunMain(VarDecAss(prim, val) + EQ)),
      PRIM_NEQ = std::make_pair("PrimitiveNotEqual" + prim, FunMain(VarDecAss(prim, val) + NEQ)),
      PRIM_LT = std::make_pair("PrimitiveLessThan" + prim, FunMain(VarDecAss(prim, val) + LT)),
      PRIM_GT = std::make_pair("PrimitiveGreaterThan" + prim, FunMain(VarDecAss(prim, val) + GT)),
      PRIM_LTE = std::make_pair("PrimitiveLessThanOrEqual" + prim, FunMain(VarDecAss(prim, val) + LTE)),
      PRIM_GTE = std::make_pair("PrimitiveGreaterThanOrEqual" + prim, FunMain(VarDecAss(prim, val) + GTE)),
      PRIM_PRE_INC = std::make_pair("VariablePrefixInc" + prim, FunMain(VarDecAss(prim, val) + PRE_INC)),
      PRIM_PRE_DEC = std::make_pair("VariablePrefixDec" + prim, FunMain(VarDecAss(prim, val) + PRE_DEC)),
      PRIM_POST_INC = std::make_pair("VariablePostfixInc" + prim, FunMain(VarDecAss(prim, val) + POST_INC)),
      PRIM_POST_DEC = std::make_pair("VariablePostfixDec" + prim, FunMain(VarDecAss(prim, val) + POST_DEC)),
      VAR_PRIM_INP_ADD = std::make_pair("VariablePrimitiveInplaceAdd" + prim, FunMain(VarDecAss(prim, val) + INP_ADD)),
      VAR_PRIM_INP_SUB = std::make_pair("VariablePrimitiveInplaceSubtract" + prim, FunMain(VarDecAss(prim, val) + INP_SUB)),
      VAR_PRIM_INP_MUL = std::make_pair("VariablePrimitiveInplaceMultiply" + prim, FunMain(VarDecAss(prim, val) + INP_MUL)),
      VAR_PRIM_INP_DIV = std::make_pair("VariablePrimitiveInplaceDivide" + prim, FunMain(VarDecAss(prim, val) + INP_DIV)),
      VAR_PRIM_INP_MOD = std::make_pair("VariablePrimitiveInplaceModulo" + prim, FunMain(VarDecAss(prim, val) + INP_MOD));

//Define{benchmark,baseline}pairs
  std::unordered_map<std::string,std::string>
      baselineMap({
                      {"ReturnValue" + prim,"Return"},
                      {"VariableDeclare" + prim,"Return"},
                      {"VariableDeclareAssign" + prim,"Return"},
                      {"PushConst" + prim,"Return"},
                      {"PushVariable" + prim,"PushConst" + prim},
                      {"PopToVariable" + prim,"VariableDeclareAssign" + prim},
                      {"PrimitiveAdd" + prim,"PushVariable" + prim},
                      {"PrimitiveSubtract" + prim,"PushVariable" + prim},
                      {"PrimitiveMultiply" + prim,"PushVariable" + prim},
                      {"PrimitiveDivide" + prim,"PushVariable" + prim},
                      {"PrimitiveModulo" + prim,"PushVariable" + prim},
                      {"PrimitiveNegate" + prim,"PushVariable" + prim},
                      {"PrimitiveEqual" + prim,"PushVariable" + prim},
                      {"PrimitiveNotEqual" + prim,"PushVariable" + prim},
                      {"PrimitiveLessThan" + prim,"PushVariable" + prim},
                      {"PrimitiveGreaterThan" + prim,"PushVariable" + prim},
                      {"PrimitiveLessThanOrEqual" + prim,"PushVariable" + prim},
                      {"PrimitiveGreaterThanOrEqual" + prim,"PushVariable" + prim},
                      {"VariablePrefixInc" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePrefixDec" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePostfixInc" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePostfixDec" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePrimitiveInplaceAdd" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePrimitiveInplaceSubtract" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePrimitiveInplaceMultiply" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePrimitiveInplaceDivide" + prim,"VariableDeclareAssign" + prim},
                      {"VariablePrimitiveInplaceModulo" + prim,"VariableDeclareAssign" + prim}
                  });

  std::vector<std::pair<std::string, std::string>>
      etch_codes = {RET_VAL, VAR_DEC, VAR_DEC_ASS, PUSH_CONST, PUSH_VAR, POP_TO_VAR, PRIM_ADD, PRIM_SUB, PRIM_MUL,
                    PRIM_DIV, PRIM_NEG, PRIM_EQ, PRIM_NEQ, PRIM_LT, PRIM_GT, PRIM_LTE, PRIM_GTE, PRIM_PRE_INC,
                    PRIM_PRE_DEC, PRIM_POST_INC, PRIM_POST_DEC, VAR_PRIM_INP_ADD, VAR_PRIM_INP_SUB, VAR_PRIM_INP_MUL,
                    VAR_PRIM_INP_DIV, VAR_PRIM_INP_MOD};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void MathBenchmarks(benchmark::State &state) {

  static std::vector<std::string> primitives{"Float32", "Float64", "Fixed32", "Fixed64"};

  static std::vector<std::string> values{"0.5f", "0.5", "0.5fp32", "0.5fp64"};

  static std::vector<std::string> alt_values{"1.5f", "1.5", "1.5fp32", "1.5fp64"};

  auto bm_ind = static_cast<u_int>(state.range(0));

  // Get the index of the primitive corresponding to the benchmark range variable
  u_int prim_ind = (bm_ind - math_begin) / n_math_bms;
  u_int etch_ind = (bm_ind - math_begin) % n_math_bms;

  std::string prim(primitives[prim_ind]);
  std::string val(values[prim_ind]);
  std::string alt_val(alt_values[prim_ind]);

  // Operations
  static std::string
      PUSH("x;\n"),
      ABS("abs(x);\n"),
      SIN("sin(x);\n"),
      COS("cos(x);\n"),
      TAN("tan(x);\n"),
      ASIN("asin(x);\n"),
      ACOS("acos(x);\n"),
      ATAN("atan(x);\n"),
      SINH("sinh(x);\n"),
      COSH("cosh(x);\n"),
      TANH("tanh(x);\n"),
      ASINH("asinh(x);\n"),
      ACOSH("acosh(x);\n"),
      ATANH("atanh(x);\n"),
      SQRT("sqrt(x);\n"),
      EXP("exp(x);\n"),
      POW("pow(x,x);\n");

  std::pair<std::string, std::string>
      PRIM_ABS = std::make_pair("PrimitiveAbs" + prim, FunMain(VarDecAss(prim, val) + ABS)),
      PRIM_SIN = std::make_pair("PrimitiveSin" + prim, FunMain(VarDecAss(prim, val) + SIN)),
      PRIM_COS = std::make_pair("PrimitiveCos" + prim, FunMain(VarDecAss(prim, val) + COS)),
      PRIM_TAN = std::make_pair("PrimitiveTan" + prim, FunMain(VarDecAss(prim, val) + TAN)),
      PRIM_ASIN = std::make_pair("PrimitiveAsin" + prim, FunMain(VarDecAss(prim, val) + ASIN)),
      PRIM_ACOS = std::make_pair("PrimitiveAcos" + prim, FunMain(VarDecAss(prim, val) + ACOS)),
      PRIM_ATAN = std::make_pair("PrimitiveAtan" + prim, FunMain(VarDecAss(prim, val) + ATAN)),
      PRIM_SINH = std::make_pair("PrimitiveSinh" + prim, FunMain(VarDecAss(prim, val) + SINH)),
      PRIM_COSH = std::make_pair("PrimitiveCosh" + prim, FunMain(VarDecAss(prim, val) + COSH)),
      PRIM_TANH = std::make_pair("PrimitiveTanh" + prim, FunMain(VarDecAss(prim, val) + TANH)),
      PRIM_ASINH = std::make_pair("PrimitiveAsinh" + prim, FunMain(VarDecAss(prim, val) + ASINH)),
      PRIM_ACOSH = std::make_pair("PrimitiveAcosh" + prim, FunMain(VarDecAss(prim, alt_val) + ACOSH)),
      PRIM_ATANH = std::make_pair("PrimitiveAtanh" + prim, FunMain(VarDecAss(prim, val) + ATANH)),
      PRIM_SQRT = std::make_pair("PrimitiveSqrt" + prim, FunMain(VarDecAss(prim, val) + SQRT)),
      PRIM_EXP = std::make_pair("PrimitiveExp" + prim, FunMain(VarDecAss(prim, val) + EXP)),
      PRIM_POW = std::make_pair("PrimitivePow" + prim, FunMain(VarDecAss(prim, val) + POW)),
      PRIM_RAND = std::make_pair("PrimitiveRand" + prim, FunMain(Rand(val, alt_val)));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"PrimitiveAbs" + prim,   "PushVariable" + prim},
                      {"PrimitiveSin" + prim,   "PushVariable" + prim},
                      {"PrimitiveCos" + prim,   "PushVariable" + prim},
                      {"PrimitiveTan" + prim,   "PushVariable" + prim},
                      {"PrimitiveAsin" + prim,  "PushVariable" + prim},
                      {"PrimitiveAcos" + prim,  "PushVariable" + prim},
                      {"PrimitiveAtan" + prim,  "PushVariable" + prim},
                      {"PrimitiveSinh" + prim,  "PushVariable" + prim},
                      {"PrimitiveCosh" + prim,  "PushVariable" + prim},
                      {"PrimitiveTanh" + prim,  "PushVariable" + prim},
                      {"PrimitiveAsinh" + prim, "PushVariable" + prim},
                      {"PrimitiveAcosh" + prim, "PushVariable" + prim},
                      {"PrimitiveAtanh" + prim, "PushVariable" + prim},
                      {"PrimitiveSqrt" + prim,  "PushVariable" + prim},
                      {"PrimitiveExp" + prim,   "PushVariable" + prim},
                      {"PrimitivePow" + prim,   "PushVariable" + prim},
                      {"PrimitiveRand" + prim,  "Return"}
                  });

  std::vector<std::pair<std::string, std::string>>
      etch_codes = {PRIM_ABS, PRIM_SIN, PRIM_COS, PRIM_TAN, PRIM_ASIN, PRIM_ACOS, PRIM_ATAN, PRIM_SINH, PRIM_COSH,
                    PRIM_TANH, PRIM_ASINH, PRIM_ACOSH, PRIM_ATANH, PRIM_SQRT, PRIM_EXP, PRIM_POW};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second,
                    baselineMap[etch_codes[etch_ind].first], bm_ind);
}

void CryptoBenchmarks(benchmark::State &state) {

  auto bm_ind = static_cast<u_int>(state.range(0));

  u_int etch_ind = bm_ind - crypto_begin;

  // Operations
  static std::string
      DEC("var s = SHA256();\n"),
      RESET("s.reset();\n"),
      FINAL("s.final();\n");

  u_int str_len = 32;

  std::pair<std::string, std::string>
      SHA256_DEC = std::make_pair("Sha256Declare", FunMain(DEC)),
      SHA256_UPDATE = std::make_pair("Sha256Update", FunMain(DEC + Sha256Update(str_len))),
      SHA256_RESET = std::make_pair("Sha256Reset", FunMain(DEC + RESET)),
      SHA256_FINAL = std::make_pair("Sha256Final", FunMain(DEC + FINAL));

  // Define {benchmark,baseline} pairs
  std::unordered_map<std::string, std::string>
      baselineMap({
                      {"Sha256Declare", "Return"},
                      {"Sha256Update",  "Sha256Declare"},
                      {"Sha256Reset",   "Sha256Declare"},
                      {"Sha256Final",   "Sha256Declare"}
                  });

  std::vector<std::pair<std::string, std::string>> etch_codes = {SHA256_DEC, SHA256_UPDATE, SHA256_RESET, SHA256_FINAL};

  EtchCodeBenchmark(state, etch_codes[etch_ind].first, etch_codes[etch_ind].second, baselineMap[etch_codes[etch_ind].first],
                    bm_ind);
}

bool RegisterBenchmarks() {
  BENCHMARK(BasicBenchmarks)->DenseRange(basic_begin, basic_end - 1, 1);
  BENCHMARK(ObjectBenchmarks)->DenseRange(object_begin, object_end - 1, 1);
  BENCHMARK(PrimitiveOpBenchmarks)->DenseRange(prim_begin, prim_end - 1, 1);
  BENCHMARK(MathBenchmarks)->DenseRange(math_begin, math_end - 1, 1);
  BENCHMARK(CryptoBenchmarks)->DenseRange(crypto_begin, crypto_end - 1, 1);
  return true;
}

bool const vm_benchmark_success = RegisterBenchmarks();

} // namespace