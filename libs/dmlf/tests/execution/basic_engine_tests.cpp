//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "gtest/gtest.h"

#include "dmlf/execution/basic_vm_engine.hpp"

#include "dmlf/execution/execution_error_message.hpp"
#include "variant/variant.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <limits>
#include <vector>

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

namespace {

namespace {

using namespace fetch::vm;
using namespace fetch::dmlf;

using Stage = ExecutionErrorMessage::Stage;
using Code  = ExecutionErrorMessage::Code;

using LedgerVariant = BasicVmEngine::LedgerVariant;
using Params        = BasicVmEngine::Params;

using fp64_t = fetch::fixed_point::fp64_t;
using fp32_t = fetch::fixed_point::fp32_t;

auto const return1 = R"(

function main() : Int32

  return 1;

endfunction)";

auto const tick = R"(

persistent tick : Int32;

function main() : Int32

  use tick;

  var result = tick.get(0);

  tick.set(tick.get(0) + 1);

  return result;

endfunction
)";

auto const tick2 = R"(

persistent tick : Int32;

function tick2() : Int32

  use tick;

  var result = tick.get(0);

  tick.set(tick.get(0) + 2);

  return result;

endfunction
)";

auto const tock = R"(

persistent tock : Int32;

function tock() : Int32

  use tock;

  var result = tock.get(0);

  tock.set(tock.get(0) + 1);

  return result;

endfunction
)";

auto const tickTock = R"(

persistent tick : Int32;
persistent tock : Int32;

function tick() : Int32

  use tick;

  var result = tick.get(0);

  tick.set(tick.get(0) + 1);

  return result;
endfunction

function tock() : Int32

  use tock;

  var result = tock.get(0);

  tock.set(tock.get(0) + 2);

  return result;
endfunction
)";

auto const badCompile = R"(

function main()

  return 1;

endfunction)";

auto const runtimeError = R"(

function main() : Int32

    var name = Array<Int32>(2);

    var a = 0;

    for (i in 0:4)
       a = name[i];
    endfor

    return 1;
endfunction)";

auto const add = R"(

 function add(a : Int32, b : Int32) : Int32
  return a + b;
 endfunction

)";

auto const Add8 = R"(

 function add(a : Int8, b : Int8) : Int8
  return a + b;
 endfunction

)";

auto const Add64 = R"(

 function add(a : Int64, b : Int64) : Int64
  return a + b;
 endfunction

)";

auto const AddFixed = R"(

function add(a : Fixed64, b : Fixed32) : Fixed64
  return a + toFixed64(b);
endfunction

)";

auto const BoolCompare = R"(
function compare(a : Bool) : Int32
  if (a)
    return 1;
  else
    return 0;
  endif
endfunction
)";

auto const AddMatrix = R"(

persistent matrix : Array<Array<Int32>>;

function init()

  use matrix;
  //var matrix = State<Array<Array<Int32>>>("matrix");

  var swa = Array<Array<Int32> >(2);
  swa[0] = Array<Int32>(2);
  swa[1] = Array<Int32>(2);

  swa[0][0] = 0;
  swa[0][1] = 1;
  swa[1][0] = 2;
  swa[1][1] = 3;

  matrix.set(swa);
endfunction

function doAdd() : Int32

  use matrix;

  var swa = matrix.get();

  return swa[0][0] + swa[0][1] +
         swa[1][0] + swa[1][1];
endfunction

)";

auto const AddMatrix2 = R"(

persistent matrix : Array<Array<Int32>>;

function init()

  use matrix;
  //var matrix = State<Array<Array<Int32>>>("matrix");

  var swa = Array<Array<Int32> >(2);
  swa[0] = Array<Int32>(2);
  swa[1] = Array<Int32>(2);

  swa[0][0] = 0;
  swa[0][1] = 1;
  swa[1][0] = 2;
  swa[1][1] = 3;

  matrix.set(swa);
endfunction

function doAdd() : Int32

  use matrix;

  var swa = matrix.get();

  return swa[0][0] + swa[0][1] +
         swa[1][0] + swa[1][1];
endfunction

)";

auto const StateMatrix = R"(
function doStuff()

    var myState = State<Array<Array<Array<Int64>>>>("arrayState");

    var test = Array<Array<Int64>>(2);

    test[0] = Array<Int64>(2);
    test[1] = Array<Int64>(2);

    test[0][0] = 0i64;
    test[0][1] = 1i64;
    test[1][0] = 2i64;
    test[1][1] = 3i64;

    var bigger = Array<Array<Array<Int64>>>(2);
    bigger[0] = test;
    bigger[1] = test;

    myState.set(bigger);

    printLn("State is " + toString(myState.get()[0][1][0]));

    changeState(myState);
    printLn("State is " + toString(myState.get()[0][1][0]));

endfunction

function doStuff2()
    otherChange();
endfunction

function main()

  doStuff();
  doStuff2();

endfunction

function changeState(state : State<Array<Array<Array<Int64>>>>)

  state.get()[0][1][0] = 5i64;

endfunction

function otherChange()

  var myState = State<Array<Array<Array<Int64>>>>("arrayState");

  myState.get()[0][1][0] = myState.get()[0][1][0] * 2i64;
  printLn("State is " + toString(myState.get()[0][1][0]));

endfunction
)";

auto const BigStMatrix = R"(

function doStuff()

    var myState = State<Array<Array<Array<Array<Int64>>>>>("arrayState");

    var test = Array<Array<Int64>>(2);

    test[0] = Array<Int64>(2);
    test[1] = Array<Int64>(2);

    test[0][0] = 0i64;
    test[0][1] = 1i64;
    test[1][0] = 2i64;
    test[1][1] = 3i64;

    var bigger = Array<Array<Array<Int64>>>(2);
    bigger[0] = test;
    bigger[1] = test;

    var evenBigger = Array<Array<Array<Array<Int64>>>>(2);
    evenBigger[0] = bigger;
    evenBigger[1] = bigger;
    myState.set(evenBigger);

    printLn("State is " + toString(myState.get()[0][0][1][0]));

    //changeState(myState);
    printLn("State is " + toString(myState.get()[0][0][1][0]));

endfunction

function doStuff2()
    otherChange();
endfunction

function main()

  doStuff();
  doStuff2();

endfunction

function changeState(state : State<Array<Array<Array<Array<Int64>>>>>)

  state.get()[0][0][1][0] = 5i64;

endfunction

function otherChange()

  var myState = State<Array<Array<Array<Array<Int64>>>>>("arrayState");

  myState.get()[0][0][1][0] = myState.get()[0][0][1][0] * 2i64;
  printLn("State is " + toString(myState.get()[0][0][1][0]));

endfunction

)";

auto const StringOut = R"(

function outString() : String
  return "Hello";
endfunction

)";

auto const IntToString = R"(
  function IntToString(x : Int32) : String
    return toString(x);
  endfunction

)";

auto const ArrayInt64Out = R"(

function arrayOut() : Array<Int64>
  var array = Array<Int64>(2);
  array[0] = 1i64;
  array[1] = 2i64;
  return array;
endfunction

)";

auto const ArrayIntInt64Out = R"(

function arrayOut() : Array<Array<Int64>>
  var array = Array<Int64>(2);
  array[0] = 1i64;
  array[1] = 2i64;

  var big = Array<Array<Int64>>(2);
  big[0] = array;
  big[1] = array;

  return big;
endfunction


)";

auto const ArrayArrayOp = R"(

function doInt8(arr : Array<Array<Int8>>) : Array<Array<Int8>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction
function doUInt8(arr : Array<Array<UInt8>>) : Array<Array<UInt8>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction

function doInt16(arr : Array<Array<Int16>>) : Array<Array<Int16>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction
function doUInt16(arr : Array<Array<UInt16>>) : Array<Array<UInt16>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction

function doInt32(arr : Array<Array<Int32>>) : Array<Array<Int32>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction
function doUInt32(arr : Array<Array<UInt32>>) : Array<Array<UInt32>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction

function doInt64(arr : Array<Array<Int64>>) : Array<Array<Int64>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction
function doUInt64(arr : Array<Array<UInt64>>) : Array<Array<UInt64>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction


function doFixed32(arr : Array<Array<Fixed32>>) : Array<Array<Fixed32>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction
function doFixed64(arr : Array<Array<Fixed64>>) : Array<Array<Fixed64>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction


function doBool(arr : Array<Array<Bool>>) : Array<Array<Bool>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction

function doString(arr : Array<Array<String>>) : Array<Array<String>>
  arr[0][0] = arr[1][1];
  return arr;
endfunction

)";

auto const ReturnArray = R"(

function ReturnArrayBool() : Array<Bool>
  var arr = Array<Bool>(2);
  arr[0] = true;
  arr[1] = false;
  return arr;
endfunction

)";

ExecutionResult RunStatelessTest(std::string const &which, std::string const &entrypoint,
                                 Params const &params)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("exec", {{"etch", which}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("exec", "state", entrypoint, params);
  return result;
}

template <typename T>
LedgerVariant Make2x2(std::vector<T> const &vals)
{
  LedgerVariant result = LedgerVariant::Array(2);
  result[0]            = LedgerVariant::Array(2);
  result[1]            = LedgerVariant::Array(2);

  result[0][0] = LedgerVariant{vals[0]};
  result[0][1] = LedgerVariant{vals[1]};
  result[1][0] = LedgerVariant{vals[2]};
  result[1][1] = LedgerVariant{vals[3]};

  return result;
}
LedgerVariant Make2x2(std::vector<std::string> const &vals)
{
  LedgerVariant result = LedgerVariant::Array(2);
  result[0]            = LedgerVariant::Array(2);
  result[1]            = LedgerVariant::Array(2);

  result[0][0] = LedgerVariant{std::string(vals[0])};
  result[0][1] = LedgerVariant{std::string(vals[1])};
  result[1][0] = LedgerVariant{std::string(vals[2])};
  result[1][1] = LedgerVariant{std::string(vals[3])};

  return result;
}

LedgerVariant Make2x2(std::vector<bool> const &vals)
{
  LedgerVariant result = LedgerVariant::Array(2);
  result[0]            = LedgerVariant::Array(2);
  result[1]            = LedgerVariant::Array(2);

  result[0][0] = LedgerVariant{static_cast<bool>(vals[0])};
  result[0][1] = LedgerVariant{static_cast<bool>(vals[1])};
  result[1][0] = LedgerVariant{static_cast<bool>(vals[2])};
  result[1][1] = LedgerVariant{static_cast<bool>(vals[3])};

  return result;
}

}  // namespace

TEST(BasicVmEngineDmlfTests, Return1)
{
  ExecutionResult result = RunStatelessTest(return1, "main", {});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 1);
}

// Check if running has sideffects on engine
TEST(BasicVmEngineDmlfTests, DoubleReturn1)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("return1", {{"etch", return1}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("return1", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("return1", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, repeated_Return1)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("return1", {{"etch", return1}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  createdProgram = engine.CreateExecutable("return1", {{"etch", return1}});
  createdState   = engine.CreateState("state");

  EXPECT_FALSE(createdProgram.succeeded());
  EXPECT_EQ(createdProgram.error().stage(), Stage::ENGINE);
  EXPECT_EQ(createdProgram.error().code(), Code::BAD_EXECUTABLE);
  EXPECT_FALSE(createdState.succeeded());
  EXPECT_EQ(createdState.error().stage(), Stage::ENGINE);
  EXPECT_EQ(createdState.error().code(), Code::BAD_STATE);

  ExecutionResult result = engine.Run("return1", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, Tick_2States)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state1");
  EXPECT_TRUE(createdState.succeeded());
  createdState = engine.CreateState("state2");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state1", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state1", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state1", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, Tick_Tick2_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = engine.CreateExecutable("tick2", {{"etch", tick2}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick2", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tick2", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("tick2", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 7);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 9);

  result = engine.Run("tick2", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 10);
}

TEST(BasicVmEngineDmlfTests, Tick_Tock_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = engine.CreateExecutable("tock", {{"etch", tock}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);
}

TEST(BasicVmEngineDmlfTests, Tick_TickTock_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = engine.CreateExecutable("tickTock", {{"etch", tickTock}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tickTock", "state", "tick", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tickTock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tickTock", "state", "tick", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tickTock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);

  result = engine.Run("tickTock", "state", "tick", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 5);

  result = engine.Run("tickTock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);
}

TEST(BasicVmEngineDmlfTests, TickState_TockState2)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = engine.CreateExecutable("tick2", {{"etch", tick2}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());
  createdState = engine.CreateState("state2");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick2", "state2", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tick2", "state2", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);

  result = engine.Run("tick2", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 5);

  result = engine.Run("tick2", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);
}

TEST(BasicVmEngineDmlfTests, Tick_Tock_TickTock_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = engine.CreateExecutable("tickTock", {{"etch", tickTock}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = engine.CreateExecutable("tock", {{"etch", tock}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tickTock", "state", "tick", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tickTock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tickTock", "state", "tick", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tickTock", "state", "tock", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);
}

TEST(BasicVmEngineDmlfTests, Tick_Tick_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = engine.CreateExecutable("tick2", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = engine.CreateExecutable("tick3", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = engine.CreateExecutable("tick4", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = engine.CreateExecutable("tick5", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = engine.CreateExecutable("tick6", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = engine.CreateExecutable("tick7", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = engine.CreateExecutable("tick8", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = engine.CreateExecutable("tick9", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick2", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick2", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);

  result = engine.Run("tick2", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 5);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("tick2", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 7);
}

TEST(BasicVmEngineDmlfTests, Tick_Tick_CopyState)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  ExecutionResult copyStateResult = engine.CopyState("state", "state2");
  EXPECT_TRUE(copyStateResult.succeeded());

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);
}

TEST(BasicVmEngineDmlfTests, CopyState_BadSrc)
{
  BasicVmEngine engine;

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult copyState = engine.CopyState("badName", "newState");
  EXPECT_FALSE(copyState.succeeded());
  EXPECT_EQ(copyState.error().stage(), Stage::ENGINE);
  EXPECT_EQ(copyState.error().code(), Code::BAD_STATE);
}

TEST(BasicVmEngineDmlfTests, CopyState_BadDest)
{
  BasicVmEngine engine;

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());
  ExecutionResult createdState2 = engine.CreateState("other");
  EXPECT_TRUE(createdState2.succeeded());

  ExecutionResult copyState = engine.CopyState("state", "other");
  EXPECT_FALSE(copyState.succeeded());
  EXPECT_EQ(copyState.error().stage(), Stage::ENGINE);
  EXPECT_EQ(copyState.error().code(), Code::BAD_DESTINATION);
}
TEST(BasicVmEngineDmlfTests, DeleteExecutable)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("return1", {{"etch", return1}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("return1", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  ExecutionResult deleteResult = engine.DeleteExecutable("goodbyeWorld");
  EXPECT_FALSE(deleteResult.succeeded());
  EXPECT_EQ(deleteResult.error().stage(), Stage::ENGINE);
  EXPECT_EQ(deleteResult.error().code(), Code::BAD_EXECUTABLE);
  result = engine.Run("return1", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  deleteResult = engine.DeleteExecutable("return1");
  EXPECT_TRUE(deleteResult.succeeded());
  result = engine.Run("return1", "state", "main", Params{});
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::ENGINE);
  EXPECT_EQ(result.error().code(), Code::BAD_EXECUTABLE);
}

TEST(BasicVmEngineDmlfTests, ReplaceExecutable)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  ExecutionResult deleteResult = engine.DeleteExecutable("tick");
  EXPECT_TRUE(deleteResult.succeeded());
  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::ENGINE);
  EXPECT_EQ(result.error().code(), Code::BAD_EXECUTABLE);

  createdProgram = engine.CreateExecutable("tick", {{"etch", tick2}});
  EXPECT_TRUE(createdProgram.succeeded());

  result = engine.Run("tick", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state", "tick2", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 4);
}

TEST(BasicVmEngineDmlfTests, Tick_Delete_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  ExecutionResult deleteState = engine.DeleteState("badState");
  EXPECT_FALSE(deleteState.succeeded());
  EXPECT_EQ(deleteState.error().stage(), Stage::ENGINE);
  EXPECT_EQ(deleteState.error().code(), Code::BAD_STATE);
  deleteState = engine.DeleteState("state");
  EXPECT_TRUE(deleteState.succeeded());

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::ENGINE);
  EXPECT_EQ(result.error().code(), Code::BAD_STATE);
}

TEST(BasicVmEngineDmlfTests, Tick_Replace_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  ExecutionResult deleteState = engine.DeleteState("state");
  EXPECT_TRUE(deleteState.succeeded());

  createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, Tick_ReplaceByCopy_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  createdState = engine.CreateState("state2");
  EXPECT_TRUE(createdState.succeeded());

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 0);

  result = engine.Run("tick", "state2", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  ExecutionResult deleteState = engine.DeleteState("state");
  EXPECT_TRUE(deleteState.succeeded());

  ExecutionResult copyState = engine.CopyState("state2", "state");
  EXPECT_TRUE(createdState.succeeded());

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 2);

  result = engine.Run("tick", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 3);
}

TEST(BasicVmEngineDmlfTests, BadCompile)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("badCompile", {{"etch", badCompile}});
  EXPECT_FALSE(createdProgram.succeeded());
  EXPECT_EQ(createdProgram.error().stage(), Stage::COMPILE);
  EXPECT_EQ(createdProgram.error().code(), Code::COMPILATION_ERROR);
}

TEST(BasicVmEngineDmlfTests, RuntimeError)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("runtime", {{"etch", runtimeError}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("runtime", "state", "main", Params{});
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::RUNNING);
  EXPECT_EQ(result.error().code(), Code::RUNTIME_ERROR);
}

TEST(BasicVmEngineDmlfTests, Add)
{
  ExecutionResult result = RunStatelessTest(add, "add", Params{LedgerVariant(1), LedgerVariant(2)});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 3);
}

TEST(BasicVmEngineDmlfTests, Add8)
{
  ExecutionResult result =
      RunStatelessTest(Add8, "add", Params{LedgerVariant(1), LedgerVariant(2)});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 3);
}

TEST(BasicVmEngineDmlfTests, Add64)
{
  ExecutionResult result = RunStatelessTest(
      Add64, "add", Params{LedgerVariant(0), LedgerVariant(std::numeric_limits<int>::max())});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), std::numeric_limits<int>::max());
}

TEST(BasicVmEngineDmlfTests, AddFixed)
{
  ExecutionResult result = RunStatelessTest(
      AddFixed, "add", Params{LedgerVariant(fp64_t("4.5")), LedgerVariant(fp32_t("5.5"))});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<fp64_t>(), static_cast<fp64_t>(10));
}

TEST(BasicVmEngineDmlfTests, TrueBoolCompare)
{
  ExecutionResult result = RunStatelessTest(BoolCompare, "compare", Params{LedgerVariant(true)});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, FalseBoolCompare)
{
  ExecutionResult result = RunStatelessTest(BoolCompare, "compare", Params{LedgerVariant(false)});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 0);
}

TEST(BasicVmEngineDmlfTests, AddMatrix)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddMatrix}});
  ASSERT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  ASSERT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("add", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, AddMatrixSameCode)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddMatrix}});
  ASSERT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
  createdProgram = engine.CreateExecutable("add2", {{"etch", AddMatrix}});
  ASSERT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  ASSERT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("add2", "state", "doAdd", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, AddMatrixEqualCode)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddMatrix}});
  ASSERT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
  createdProgram = engine.CreateExecutable("add2", {{"etch", AddMatrix2}});
  ASSERT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  ASSERT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("add2", "state", "doAdd", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, StateMatrixMain)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", StateMatrix}});
  ASSERT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  ASSERT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "main", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, StateMatrixMyCalls)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", StateMatrix}});
  ASSERT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  ASSERT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "doStuff", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("stateMatrix", "state", "doStuff2", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, BigStateMatrixMain)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", BigStMatrix}});
  ASSERT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  ASSERT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "main", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, BigStateMatrixMyCalls)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", BigStMatrix}});
  ASSERT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  ASSERT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "doStuff", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("stateMatrix", "state", "doStuff2", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, StringOutput)
{
  ExecutionResult result = RunStatelessTest(StringOut, "outString", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<std::string>(), "Hello");
}

TEST(BasicVmEngineDmlfTests, IntToString)
{
  ExecutionResult result = RunStatelessTest(IntToString, "IntToString", Params{LedgerVariant(1)});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<std::string>(), "1");
}

TEST(BasicVmEngineDmlfTests, ArrayInt64)
{
  ExecutionResult result = RunStatelessTest(ArrayInt64Out, "arrayOut", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  auto output = result.output();
  EXPECT_TRUE(output.IsArray());
  EXPECT_TRUE(output.size() == 2);
  EXPECT_TRUE(output[0].As<int>() == 1);
  EXPECT_TRUE(output[1].As<int>() == 2);
}

TEST(BasicVmEngineDmlfTests, ArrayIntInt64)
{
  ExecutionResult result = RunStatelessTest(ArrayIntInt64Out, "arrayOut", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  auto output = result.output();
  ASSERT_TRUE(output.IsArray());
  ASSERT_EQ(output.size(), 2);
  ASSERT_TRUE(output[0].IsArray());
  ASSERT_EQ(output[0].size(), 2);
  ASSERT_TRUE(output[1].IsArray());

  EXPECT_EQ(output[1].size(), 2);
  EXPECT_EQ(output[0][0].As<int>(), 1);
  EXPECT_EQ(output[0][1].As<int>(), 2);
  EXPECT_EQ(output[1][0].As<int>(), 1);
  EXPECT_EQ(output[1][1].As<int>(), 2);
}

template <typename T>
void RunArrayTest(std::string const &entrypoint, std::vector<T> const &vals)
{
  LedgerVariant input = Make2x2(vals);

  ExecutionResult result = RunStatelessTest(ArrayArrayOp, entrypoint, Params{input});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  auto output = result.output();
  ASSERT_TRUE(output.IsArray());
  ASSERT_EQ(output.size(), 2);
  ASSERT_TRUE(output[0].IsArray());
  ASSERT_EQ(output[0].size(), 2);
  ASSERT_TRUE(output[1].IsArray());
  ASSERT_EQ(output[1].size(), 2);

  EXPECT_EQ(output[0][0].As<T>(), vals[3]);
  EXPECT_EQ(output[0][1].As<T>(), vals[1]);
  EXPECT_EQ(output[1][0].As<T>(), vals[2]);
  EXPECT_EQ(output[1][1].As<T>(), vals[3]);
}
void RunArrayTest(std::string const &entrypoint, std::vector<fp32_t> const &vals)
{
  LedgerVariant input = Make2x2(vals);

  ExecutionResult result = RunStatelessTest(ArrayArrayOp, entrypoint, Params{input});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  auto output = result.output();
  ASSERT_TRUE(output.IsArray());
  ASSERT_EQ(output.size(), 2);
  ASSERT_TRUE(output[0].IsArray());
  ASSERT_EQ(output[0].size(), 2);
  ASSERT_TRUE(output[1].IsArray());
  ASSERT_EQ(output[1].size(), 2);

  EXPECT_NEAR(static_cast<double>(output[0][0].As<fp64_t>()), static_cast<double>(vals[3]),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(output[0][1].As<fp64_t>()), static_cast<double>(vals[1]),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(output[1][0].As<fp64_t>()), static_cast<double>(vals[2]),
              static_cast<double>(fp32_t::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(output[1][1].As<fp64_t>()), static_cast<double>(vals[3]),
              static_cast<double>(fp32_t::TOLERANCE));
}

TEST(BasicVmEngineDmlfTests, ArrayArrayOpTests)
{
  RunArrayTest("doInt8", std::vector<int8_t>{1, 2, 3, 4});
  RunArrayTest("doUInt8", std::vector<uint8_t>{1, 2, 3, 4});
  RunArrayTest("doInt16", std::vector<int16_t>{1, 2, 3, 4});
  RunArrayTest("doUInt16", std::vector<uint16_t>{1, 2, 3, 4});
  RunArrayTest("doInt32", std::vector<int32_t>{1, 2, 3, 4});
  RunArrayTest("doUInt32", std::vector<uint32_t>{1, 2, 3, 4});
  RunArrayTest("doInt64", std::vector<int64_t>{1, 2, 3, 4});
  RunArrayTest("doUInt64", std::vector<uint64_t>{1, 2, 3, 4});

  RunArrayTest(
      "doFixed32",
      std::vector<fp32_t>{fetch::math::AsType<fp32_t>(1.2), fetch::math::AsType<fp32_t>(2.4),
                          fetch::math::AsType<fp32_t>(3.7), fetch::math::AsType<fp32_t>(4.8)});
  RunArrayTest(
      "doFixed64",
      std::vector<fp64_t>{fetch::math::AsType<fp64_t>(1.3), fetch::math::AsType<fp64_t>(2.2),
                          fetch::math::AsType<fp64_t>(3.5), fetch::math::AsType<fp64_t>(4.7)});

  // RunArrayTest("doBool", std::vector<bool>{true,true,false,false});
  RunArrayTest("doString", std::vector<std::string>{"a", "b", "c", "d"});
}

TEST(DISABLED_BasicVmEngineDmlfTests, DisabledArrayArrayOpTests)
{
  RunArrayTest("doBool", std::vector<bool>{true, true, false, false});
  EXPECT_EQ(1, 1);
}

TEST(DISABLED_BasicVmEngineDmlfTests, ReturnArrayBool)
{
  ExecutionResult result = RunStatelessTest(ReturnArray, "ReturnArrayBool", Params{});
  ASSERT_TRUE(result.succeeded()) << result.error().message() << '\n';
  auto output = result.output();
  ASSERT_TRUE(output.IsArray());
  ASSERT_EQ(output.size(), 2);
  EXPECT_EQ(output[0].As<bool>(), true);
  EXPECT_EQ(output[1].As<bool>(), false);
}

}  // namespace
