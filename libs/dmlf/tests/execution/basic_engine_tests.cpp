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

namespace {

namespace {

using namespace fetch::vm;
using namespace fetch::dmlf;

using Stage = ExecutionErrorMessage::Stage;
using Code  = ExecutionErrorMessage::Code;

// using Params = fetch::dmlf::BasicVmEngine::Params;

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

// auto const add = R"(
//
// function add(a : Int32, b : Int32)
//  printLn("Add " + toString(a) + " plus " + toString(b) + " equals " + toString(a+b));
// endfunction
//
//)";

}  // namespace

TEST(BasicVmEngineDmlfTests, HelloWorld)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("helloWorld", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, DoubleHelloWorld)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("helloWorld", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("helloWorld", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
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

  ExecutionResult result = engine.Run("helloWorld", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
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

  ExecutionResult result = engine.Run("tick", "state1", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state1", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state1", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
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
//  ExecutionResult result = engine.Run("helloWorld",  "state", "main");
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
//  ExecutionResult result = engine.Run("helloWorld",  "state", "main");
//  EXPECT_TRUE(result.succeeded());
//  EXPECT_EQ(result.output().Get<int>(), 1);
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick2", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tick2", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 6);

  result = engine.Run("tick2", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 7);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 9);

  result = engine.Run("tick2", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 10);
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tickTock", "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tickTock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tickTock", "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tickTock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = engine.Run("tickTock", "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 5);

  result = engine.Run("tickTock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick2", "state2", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tick2", "state2", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = engine.Run("tick2", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 5);

  result = engine.Run("tick2", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tickTock", "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tickTock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tickTock", "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tickTock", "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick2", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick2", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = engine.Run("tick2", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 5);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 6);

  result = engine.Run("tick2", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 7);
}

TEST(BasicVmEngineDmlfTests, Tick_Tick_CopyState)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  ExecutionResult copyStateResult = engine.CopyState("state", "state2");
  EXPECT_TRUE(copyStateResult.succeeded());

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);
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

  ExecutionResult result = engine.Run("helloWorld", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  ExecutionResult deleteResult = engine.DeleteExecutable("goodbyeWorld");
  EXPECT_FALSE(deleteResult.succeeded());
  EXPECT_EQ(deleteResult.error().stage(), Stage::ENGINE);
  EXPECT_EQ(deleteResult.error().code(), Code::BAD_EXECUTABLE);
  result = engine.Run("helloWorld", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  deleteResult = engine.DeleteExecutable("helloWorld");
  EXPECT_TRUE(deleteResult.succeeded());
  result = engine.Run("helloWorld", "state", "main");
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  ExecutionResult deleteResult = engine.DeleteExecutable("tick");
  EXPECT_TRUE(deleteResult.succeeded());
  result = engine.Run("tick", "state", "main");
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::ENGINE);
  EXPECT_EQ(result.error().code(), Code::BAD_EXECUTABLE);

  createdProgram = engine.CreateExecutable("tick", {{"etch", tick2}});
  EXPECT_TRUE(createdProgram.succeeded());

  result = engine.Run("tick", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);
}

TEST(BasicVmEngineDmlfTests, Tick_Delete_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  ExecutionResult deleteState = engine.DeleteState("badState");
  EXPECT_FALSE(deleteState.succeeded());
  EXPECT_EQ(deleteState.error().stage(), Stage::ENGINE);
  EXPECT_EQ(deleteState.error().code(), Code::BAD_STATE);
  deleteState = engine.DeleteState("state");
  EXPECT_TRUE(deleteState.succeeded());

  result = engine.Run("tick", "state", "main");
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

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  ExecutionResult deleteState = engine.DeleteState("state");
  EXPECT_TRUE(deleteState.succeeded());

  createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
}

TEST(BasicVmEngineDmlfTests, Tick_ReplaceByCopy_State)
{
  BasicVmEngine engine;

  ExecutionResult createdProgram = engine.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = engine.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  createdState = engine.CreateState("state2");
  EXPECT_TRUE(createdState.succeeded());

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = engine.Run("tick", "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  ExecutionResult deleteState = engine.DeleteState("state");
  EXPECT_TRUE(deleteState.succeeded());

  ExecutionResult copyState = engine.CopyState("state2", "state");
  EXPECT_TRUE(createdState.succeeded());

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = engine.Run("tick", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);
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

  ExecutionResult result = engine.Run("runtime", "state", "main");
  EXPECT_FALSE(result.succeeded());
  EXPECT_EQ(result.error().stage(), Stage::RUNNING);
  EXPECT_EQ(result.error().code(), Code::RUNTIME_ERROR);
}

// TEST(BasicVmEngineDmlfTests, params)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  Params params{Variant(5, TypeIds::Int32), Variant(6, TypeIds::Int32)};
//  ExecutionResult result = engine.Run("add",  "state", "add", params);
//
//  EXPECT_TRUE(result.succeeded());
//
//  //EXPECT_EQ(output.str(), "Add 5 plus 6 equals 11\n");
//}

// TEST(BasicVmEngineDmlfTests, less_params)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{Variant(5, TypeIds::Int32)};
//  ExecutionResult result = engine.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage, "mismatched parameters: expected 2 arguments, but got 1");
//}
//
// TEST(BasicVmEngineDmlfTests, more_params)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{Variant(5, TypeIds::Int32), Variant(5, TypeIds::Int32), Variant(5,
//  TypeIds::Int32)}; ExecutionResult result = engine.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage, "mismatched parameters: expected 2 arguments, but got 3");
//}
//
// TEST(BasicVmEngineDmlfTests, none_params)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{};
//  ExecutionResult result = engine.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage, "mismatched parameters: expected 2 arguments, but got 0");
//}
//
// TEST(BasicVmEngineDmlfTests, wrong_type_params)
//{
//  BasicVmEngine engine;
//
//  ExecutionResult createdProgram = engine.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = engine.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{Variant(5, TypeIds::Float32), Variant(5, TypeIds::Int32)};
//  ExecutionResult result = engine.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage,
//            "mismatched parameters: expected argument 0to be of type Int32 but got Float32");
//}

}  // namespace
