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

auto const helloWorld = R"(

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

auto const AddFloat = R"(

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

auto const AddArray = R"(

function add(array : Array<Int32>) : Int32
  return array[0] + array[1];
endfunction

)";

auto const AddArrayThree = R"(

function add(array : Array<Int32>) : Int32
  return array[0] + array[1] + array[2];
endfunction

)";

auto const Add3Array = R"(

function add(array2 : Array<Array<Array<Int32>>>, array : Array<Int32>) : Int32
  return array[0] + array[1] + array2[0][1][2];
endfunction

)";

}  // namespace

TEST(BasicVmEngineDmlfTests, HelloWorld)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("helloWorld", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, DoubleHelloWorld)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("helloWorld", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  result = engine.Run("helloWorld", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, repeated_HelloWorld)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  createdState   = engine.CreateState("state");

  EXPECT_FALSE(createdProgram.succeeded());
  EXPECT_EQ(createdProgram.error().stage(), Stage::ENGINE);
  EXPECT_EQ(createdProgram.error().code(), Code::BAD_EXECUTABLE);
  EXPECT_FALSE(createdState.succeeded());
  EXPECT_EQ(createdState.error().stage(), Stage::ENGINE);
  EXPECT_EQ(createdState.error().code(), Code::BAD_STATE);

  ExecutionResult result = engine.Run("helloWorld", "state", "main", Params{});
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
//  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = engine.Run("helloWorld",  "state", "main", Params{});
//  EXPECT_TRUE(result.succeeded());
//
//  //EXPECT_EQ(output.str(), "Hello world!!\n");
//}

// TEST(BasicVmEngineDmlfTests, bad_stdOut)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//  std::stringstream badOutput;
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = engine.Run("helloWorld",  "state", "main", Params{});
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

  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("helloWorld", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  ExecutionResult deleteResult = engine.DeleteExecutable("goodbyeWorld");
  EXPECT_FALSE(deleteResult.succeeded());
  EXPECT_EQ(deleteResult.error().stage(), Stage::ENGINE);
  EXPECT_EQ(deleteResult.error().code(), Code::BAD_EXECUTABLE);
  result = engine.Run("helloWorld", "state", "main", Params{});
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().As<int>(), 1);

  deleteResult = engine.DeleteExecutable("helloWorld");
  EXPECT_TRUE(deleteResult.succeeded());
  result = engine.Run("helloWorld", "state", "main", Params{});
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

  ExecutionResult result = engine.Run(
      "add", "state", "add", Params{LedgerVariant(fp64_t(4.5)), LedgerVariant(fp32_t(5.5))});
  EXPECT_TRUE(result.succeeded());
  // std::cout << result.error().message() << '\n';
  EXPECT_EQ(result.output().As<fp64_t>(), 9.5);
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

TEST(BasicVmEngineDmlfTests, AddArray)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddArray}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run(
      "add", "state", "add", Params{LedgerVariant()});
  //EXPECT_TRUE(result.succeeded());
  std::cout << result.error().message() << '\n';
  //EXPECT_EQ(result.output().As<fp64_t>(), 9.5);
}

TEST(BasicVmEngineDmlfTests, AddArrayThree)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", AddArrayThree}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run(
      "add", "state", "add", Params{LedgerVariant()});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  //EXPECT_EQ(result.output().As<fp64_t>(), 9.5);
}

TEST(BasicVmEngineDmlfTests, Add3Array)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", Add3Array}});
  EXPECT_TRUE(createdProgram.succeeded()) << createdProgram.error().message() << '\n';

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run(
      "add", "state", "add", Params{LedgerVariant(), LedgerVariant()});
  EXPECT_TRUE(result.succeeded()) << result.error().message() << '\n';
  //EXPECT_EQ(result.output().As<fp64_t>(), 9.5);
}

}  // namespace
