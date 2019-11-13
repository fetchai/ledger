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

#include "gtest/gtest.h"

#include "dmlf/execution/basic_vm_engine.hpp"

#include "dmlf/execution/execution_error_message.hpp"
#include "variant/variant.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <limits>

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

auto const AddFloat        = R"(

function add(a : Float64, b : Float64) : Float64
  return a + b;
endfunction

)";
auto const AddFloatComplex = R"(

function add(a : Float64, b : Float32) : Float64
  return a + toFloat64(b);
endfunction

)";
auto const AddFloat32      = R"(

function add(a : Float32, b : Float32) : Float32
  return a + b;
endfunction

)";

auto const AddFixed = R"(

function add(a : Fixed64, b : Fixed32) : Fixed64
  return a + toFixed64(b);
endfunction

)";

auto const IntToFloatCompare = R"(
function compare(a : Int32, b: Float64) : Int32
  if (a < toInt32(b))
    return 1;
  else
    return 0;
  endif
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

auto const AddMatrixAltCode = R"(

persistent matrix1 : Array<Array<Float32>>;
persistent matrix2 : Array<Array<Int64>>;
persistent matrix  : Array<Array<Int32>>;

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

auto const AddNMatrix = R"(

persistent matrix  : Array<Array<Array<Int32>>>;

function init()

  use matrix;

  var swa = Array<Array<Array<Int32> > >(1);
  swa[0] = Array<Array<Int32> >(2);
	swa[0][0] = Array<Int32>(2);
  swa[0][1] = Array<Int32>(2);

  swa[0][0][0] = 0;
  swa[0][0][1] = 1;
  swa[0][1][0] = 2;
  swa[0][1][1] = 3;

  matrix.set(swa);
endfunction

function doAdd() : Int32

  use matrix;

  var swa = matrix.get();

  return swa[0][0][0] + swa[0][0][1] +
         swa[0][1][0] + swa[0][1][1];
endfunction

)";

auto const AddNMatrixAltCode = R"(

persistent matrix1 : Array<Array<Array<Float32>>>;
persistent matrix2 : Array<Array<Array<Int64>>>;
persistent matrix  : Array<Array<Array<Int32>>>;

function doSum1(swa : Array<Array<Array<Float32>>>) : Float32
  return swa[0][0][0];
endfunction
function doSum2(swa : Array<Array<Array<Int64>>>) : Int64
  return swa[0][0][0];
endfunction

function doSum(swa : Array<Array<Array<Int32>>>) : Int32
  return swa[0][0][0] + swa[0][0][1] +
         swa[0][1][0] + swa[0][1][1];
endfunction

function doAdd() : Int32
  use matrix1;
  use matrix2;

	var helper1 = Array<Array<Array<Float32>>>(1);
	helper1[0] = Array<Array<Float32>>(1);
	helper1[0][0] = Array<Float32>(1);
	helper1[0][0][0] = 6.0f;
	matrix1.set(helper1);

	var helper2 = Array<Array<Array<Int64>>>(1);
	helper2[0] = Array<Array<Int64>>(1);
	helper2[0][0] = Array<Int64>(1);
	helper2[0][0][0] = 6i64;
	matrix2.set(helper2);

  use matrix;
  var swa = matrix.get();

	doSum1(helper1);
	doSum2(helper2);

	return doSum(swa);

endfunction

)";

auto const AddNonPersistentMatrix = R"(

function doSum1(swa : Array<Array<Array<Float32>>>) : Float32
  return swa[0][0][0];
endfunction
function doSum2(swa : Array<Array<Array<Int64>>>) : Int64
  return swa[0][0][0];
endfunction

function doSum(swa : Array<Array<Array<Int32>>>) : Int32
  return swa[0][0][0] + swa[0][0][1] +
         swa[0][1][0] + swa[0][1][1];
endfunction

function doAdd() : Int32

	var helper1 = Array<Array<Array<Float32>>>(1);
	helper1[0] = Array<Array<Float32>>(1);
	helper1[0][0] = Array<Float32>(1);
	helper1[0][0][0] = 6.0f;

	var helper2 = Array<Array<Array<Int64>>>(1);
	helper2[0] = Array<Array<Int64>>(1);
	helper2[0][0] = Array<Int64>(1);
	helper2[0][0][0] = 6i64;

	doSum1(helper1);
	doSum2(helper2);

  var swa = Array<Array<Array<Int32> > >(1);
  swa[0] = Array<Array<Int32> >(2);
	swa[0][0] = Array<Int32>(2);
  swa[0][1] = Array<Int32>(2);

  swa[0][0][0] = 0;
  swa[0][0][1] = 1;
  swa[0][1][0] = 2;
  swa[0][1][1] = 3;

	return doSum(swa);

endfunction

)";

auto const AddNonPersistentMatrix2 = R"(
function doSum(hola : Array<Array<Array<Int32>>>) : Int32
  return hola[0][0][0] + hola[0][0][1] +
         hola[0][1][0] + hola[0][1][1];
endfunction

function doAdd() : Int32

  var swola = Array<Array<Array<Int32> > >(1);
  swola[0] = Array<Array<Int32> >(2);
	swola[0][0] = Array<Int32>(2);
  swola[0][1] = Array<Int32>(2);

  swola[0][0][0] = 0;
  swola[0][0][1] = 1;
  swola[0][1][0] = 2;
  swola[0][1][1] = 3;

	return doSum(swola);

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

}  // namespace

TEST(BasicVmEngineDmlfTests, Return1)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("return1", {{"etch", return1}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("return1", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);
}

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

// Repeated things

// Breaks VM
// TEST(BasicVmEngineDmlfTests, break_vm)
//{
//
//
//  ExecutionResult createdProgram = engine.CreateExecutable("return1", {{"etch", return1}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = engine.Run("return1",  "state", "main", Params{});
//  EXPECT_TRUE(result.succeeded());
//
//  //EXPECT_EQ(output.str(), "Hello world!!\n");
//}

// TEST(BasicVmEngineDmlfTests, bad_stdOut)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("return1", {{"etch", return1}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//  std::stringstream badOutput;
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = engine.Run("return1",  "state", "main", Params{});
//  EXPECT_TRUE(result.succeeded());
//  EXPECT_EQ(result.output().As<int>(), 1);
//}

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
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", add}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result =
      engine.Run("add", "state", "add", Params{LedgerVariant(1), LedgerVariant(2)});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 3);
}

TEST(BasicVmEngineDmlfTests, Add8)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", Add8}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result =
      engine.Run("add", "state", "add", Params{LedgerVariant(1), LedgerVariant(2)});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 3);
}

TEST(BasicVmEngineDmlfTests, Add64)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", Add64}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result =
      engine.Run("add", "state", "add",
                 Params{LedgerVariant(0), LedgerVariant(std::numeric_limits<int>::max())});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), std::numeric_limits<int>::max());
}

TEST(BasicVmEngineDmlfTests, AddFloat)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddFloat}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  double a = 4.5;
  float  b = 3.5;

  ExecutionResult result =
      engine.Run("add", "state", "add", Params{LedgerVariant(a), LedgerVariant(b)});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_NEAR(result.output().As<float>(), 8.0, 0.001);
}
TEST(BasicVmEngineDmlfTests, AddFloat32)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddFloat32}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  float a = 4.6f;
  float b = 3.5f;

  ExecutionResult result =
      engine.Run("add", "state", "add", Params{LedgerVariant(a), LedgerVariant(b)});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_NEAR(result.output().As<float>(), 8.1, 0.001);
}

TEST(BasicVmEngineDmlfTests, AddFloatComplex)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddFloatComplex}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  double a = 4.5;
  float  b = 3.3f;

  ExecutionResult result =
      engine.Run("add", "state", "add", Params{LedgerVariant(a), LedgerVariant(b)});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_NEAR(result.output().As<double>(), 7.8, 0.001);
}

TEST(BasicVmEngineDmlfTests, AddFixed)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddFixed}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run(
      "add", "state", "add", Params{LedgerVariant(fp64_t(4.5)), LedgerVariant(fp32_t(5.5))});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<fp64_t>(), 10.0);
}

TEST(BasicVmEngineDmlfTests, TrueIntToFloatCompare)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram =
      engine.CreateExecutable("compare", {{"etch", IntToFloatCompare}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result =
      engine.Run("compare", "state", "compare", Params{LedgerVariant(5), LedgerVariant(6.5)});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, FalseIntToFloatCompare)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram =
      engine.CreateExecutable("compare", {{"etch", IntToFloatCompare}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result =
      engine.Run("compare", "state", "compare", Params{LedgerVariant(5), LedgerVariant(4.5)});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 0);
}

TEST(BasicVmEngineDmlfTests, TrueBoolCompare)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("compare", {{"etch", BoolCompare}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("compare", "state", "compare", Params{LedgerVariant(true)});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, FalseBoolCompare)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("compare", {{"etch", BoolCompare}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("compare", "state", "compare", Params{LedgerVariant(false)});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 0);
}

TEST(BasicVmEngineDmlfTests, BadParamsTrueIntToFloatCompare)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram =
      engine.CreateExecutable("compare", {{"etch", IntToFloatCompare}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result =
      engine.Run("compare", "state", "compare", Params{LedgerVariant(6.5), LedgerVariant(5)});
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::ENGINE);
  EXPECT_EQ(result.error().code(), Code::RUNTIME_ERROR);
}

TEST(BasicVmEngineDmlfTests, WrongNumberOfParamsTrueIntToFloatCompare)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram =
      engine.CreateExecutable("compare", {{"etch", IntToFloatCompare}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("compare", "state", "compare", Params{LedgerVariant(6.5)});
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::ENGINE);
  EXPECT_EQ(result.error().code(), Code::RUNTIME_ERROR);
}

// TEST(BasicVmEngineDmlfTests, AddArray)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddArray}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = engine.Run(
//      "add", "state", "add", Params{LedgerVariant()});
//  //EXPECT_TRUE(result.succeeded());
//  std::cout << result.error().message() << '\n';
//  //EXPECT_EQ(result.output().As<fp64_t>(), 9.5);
//}
//
// TEST(BasicVmEngineDmlfTests, AddArrayThree)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddArrayThree}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = engine.Run(
//      "add", "state", "add", Params{LedgerVariant()});
//  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
//  //EXPECT_EQ(result.output().As<fp64_t>(), 9.5);
//}
//
// TEST(BasicVmEngineDmlfTests, Add3Array)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", Add3Array}});
//  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = engine.Run(
//      "add", "state", "add", Params{LedgerVariant(), LedgerVariant()});
//  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
//  //EXPECT_EQ(result.output().As<fp64_t>(), 9.5);
//}

TEST(BasicVmEngineDmlfTests, AddMatrix)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddMatrix}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';

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
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
  createdProgram = engine.CreateExecutable("add2", {{"etch", AddMatrix}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("add2", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, AddMatrixEqualCode)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddMatrix}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
  createdProgram = engine.CreateExecutable("add2", {{"etch", AddMatrix2}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("add2", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, AddMatrixAltCode)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddMatrix}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
  createdProgram = engine.CreateExecutable("add2", {{"etch", AddMatrixAltCode}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("add2", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, AddNMatrixAltCode)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddNMatrix}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
  createdProgram = engine.CreateExecutable("add2", {{"etch", AddNMatrixAltCode}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add", "state", "init", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("add2", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, AddNonPersistentMatrix)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram =
      engine.CreateExecutable("add", {{"etch", AddNonPersistentMatrix}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';
  createdProgram = engine.CreateExecutable("add2", {{"etch", AddNonPersistentMatrix2}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded()) << createdState.error().message() << '\n';

  ExecutionResult result = engine.Run("add2", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);

  result = engine.Run("add", "state", "doAdd", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<int>(), 6);
}

TEST(BasicVmEngineDmlfTests, StateMatrixMain)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", StateMatrix}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, StateMatrixMyCalls)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", StateMatrix}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "doStuff", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("stateMatrix", "state", "doStuff2", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, BigStateMatrixMain)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", BigStMatrix}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, BigStateMatrixMyCalls)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stateMatrix", {{"etch", BigStMatrix}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stateMatrix", "state", "doStuff", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';

  result = engine.Run("stateMatrix", "state", "doStuff2", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
}

TEST(BasicVmEngineDmlfTests, StringOutput)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("stringOut", {{"etch", StringOut}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("stringOut", "state", "outString", Params{});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<std::string>(), "Hello");
}

TEST(BasicVmEngineDmlfTests, IntToString)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("toString", {{"etch", IntToString}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("toString", "state", "IntToString", Params{LedgerVariant(1)});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<std::string>(), "1");
}

}  // namespace
