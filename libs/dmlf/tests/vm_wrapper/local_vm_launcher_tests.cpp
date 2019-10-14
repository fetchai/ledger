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

#include "dmlf/local_vm_launcher.hpp"

#include "variant/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace {

using namespace fetch::vm;
using namespace fetch::dmlf;

using Params = fetch::dmlf::LocalVmLauncher::Params;
// using Status = fetch::dmlf::VmWrapperInterface::Status;

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

//auto const add = R"(
//
//function add(a : Int32, b : Int32)
//  printLn("Add " + toString(a) + " plus " + toString(b) + " equals " + toString(a+b));
//endfunction
//
//)";

}  // namespace

TEST(VmLauncherDmlfTests, local_HelloWorld)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("helloWorld", "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
}

TEST(VmLauncherDmlfTests, local_DoubleHelloWorld)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("helloWorld",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = launcher.Run("helloWorld",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
}

TEST(VmLauncherDmlfTests, repeated_HelloWorld)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  createdProgram = launcher.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  createdState   = launcher.CreateState("state");

  EXPECT_FALSE(createdProgram.succeeded());
  EXPECT_FALSE(createdState.succeeded());

  ExecutionResult result = launcher.Run("helloWorld",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
}

TEST(VmLauncherDmlfTests, local_Tick_VM_2States)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state1");
  EXPECT_TRUE(createdState.succeeded());
  createdState = launcher.CreateState("state2");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state1", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tick",  "state1", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = launcher.Run("tick",  "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tick",  "state1", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = launcher.Run("tick",  "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
}

// Repeated things

// Breaks VM
// TEST(VmLauncherDmlfTests, break_vm)
//{
//
//
//  ExecutionResult createdProgram = launcher.CreateExecutable("helloWorld", {{"etch", helloWorld}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = launcher.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  ExecutionResult result = launcher.Run("helloWorld",  "state", "main");
//  EXPECT_TRUE(result.succeeded());
//
//  //EXPECT_EQ(output.str(), "Hello world!!\n");
//}

TEST(VmLauncherDmlfTests, bad_stdOut)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("helloWorld", {{"etch", helloWorld}});
  EXPECT_TRUE(createdProgram.succeeded());

  std::stringstream badOutput;

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("helloWorld",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);
}

TEST(VmLauncherDmlfTests, local_Tick_Tick2_VM_State)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = launcher.CreateExecutable("tick2", {{"etch", tick2}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tick2",  "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = launcher.Run("tick2",  "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 6);

  result = launcher.Run("tick2",  "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 7);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 9);

  result = launcher.Run("tick2",  "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 10);


  //EXPECT_EQ(output.str(), "0\n1\n3\n4\n6\n7\n9\n10\n");
}

TEST(VmLauncherDmlfTests, test_Tick_Tock_VM_State)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());


  createdProgram = launcher.CreateExecutable("tock", {{"etch", tock}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = launcher.Run("tock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = launcher.Run("tock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = launcher.Run("tock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);


  //EXPECT_EQ(output.str(), "0\n0\n1\n1\n2\n2\n3\n3\n");
}

TEST(VmLauncherDmlfTests, test_Tick_TickTock_VM_State)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());


  createdProgram = launcher.CreateExecutable("tickTock", {{"etch", tickTock}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tickTock",  "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = launcher.Run("tickTock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = launcher.Run("tickTock",  "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = launcher.Run("tickTock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = launcher.Run("tickTock",  "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 5);

  result = launcher.Run("tickTock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);


  //EXPECT_EQ(output.str(), "0\n1\n0\n2\n3\n2\n4\n5\n4\n");
}

TEST(VmLauncherDmlfTests, test_TickState_TockState2_VM)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());


  createdProgram = launcher.CreateExecutable("tick2", {{"etch", tick2}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());
  createdState = launcher.CreateState("state2");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tick2",  "state2", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = launcher.Run("tick2",  "state2", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);


  result = launcher.Run("tick",  "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = launcher.Run("tick2",  "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);


  result = launcher.Run("tick",  "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 5);

  result = launcher.Run("tick2",  "state", "tick2");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);


  //EXPECT_EQ(output.str(), "0\n0\n1\n2\n4\n2\n5\n4\n");
}

TEST(VmLauncherDmlfTests, test_Tick_Tock_TickTock_VM_State)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());


  createdProgram = launcher.CreateExecutable("tickTock", {{"etch", tickTock}});
  EXPECT_TRUE(createdProgram.succeeded());

  createdProgram = launcher.CreateExecutable("tock", {{"etch", tock}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tickTock",  "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);

  result = launcher.Run("tickTock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = launcher.Run("tock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = launcher.Run("tickTock",  "state", "tick");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = launcher.Run("tickTock",  "state", "tock");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);


  //EXPECT_EQ(output.str(), "0\n0\n1\n1\n2\n3\n3\n4\n");
}

TEST(VmLauncherDmlfTests, local_Tick_Tick_VM_State)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  std::cout << createdProgram.error().message() << "\n\n";


  createdProgram = launcher.CreateExecutable("tick2", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = launcher.CreateExecutable("tick3", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = launcher.CreateExecutable("tick4", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = launcher.CreateExecutable("tick5", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = launcher.CreateExecutable("tick6", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = launcher.CreateExecutable("tick7", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = launcher.CreateExecutable("tick8", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());
  createdProgram = launcher.CreateExecutable("tick9", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());

  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tick2",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = launcher.Run("tick2",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = launcher.Run("tick2",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 5);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 6);

  result = launcher.Run("tick2",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 7);


  //EXPECT_EQ(output.str(), "0\n1\n2\n3\n4\n5\n6\n7\n");
}

TEST(VmLauncherDmlfTests, local_Tick_Tick_VM_CopyState)
{
  LocalVmLauncher launcher;

  ExecutionResult createdProgram = launcher.CreateExecutable("tick", {{"etch", tick}});
  EXPECT_TRUE(createdProgram.succeeded());


  ExecutionResult createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState.succeeded());

  ExecutionResult result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 0);

  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 1);


  launcher.CopyState2("state", "state2");

  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);

  result = launcher.Run("tick",  "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 2);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);

  result = launcher.Run("tick",  "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 3);


  result = launcher.Run("tick",  "state", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);

  result = launcher.Run("tick",  "state2", "main");
  EXPECT_TRUE(result.succeeded());
  EXPECT_EQ(result.output().Get<int>(), 4);


  //EXPECT_EQ(output.str(), "0\n1\n2\n2\n3\n3\n4\n4\n");
}

//TEST(VmLauncherDmlfTests, local_params)
//{
//  LocalVmLauncher launcher;
//
//  ExecutionResult createdProgram = launcher.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = launcher.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  Params params{Variant(5, TypeIds::Int32), Variant(6, TypeIds::Int32)};
//  ExecutionResult result = launcher.Run("add",  "state", "add", params);
//
//  EXPECT_TRUE(result.succeeded());
//
//  //EXPECT_EQ(output.str(), "Add 5 plus 6 equals 11\n");
//}

//TEST(VmLauncherDmlfTests, local_less_params)
//{
//  LocalVmLauncher launcher;
//
//  ExecutionResult createdProgram = launcher.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = launcher.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{Variant(5, TypeIds::Int32)};
//  ExecutionResult result = launcher.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage, "mismatched parameters: expected 2 arguments, but got 1");
//}
//
//TEST(VmLauncherDmlfTests, local_more_params)
//{
//  LocalVmLauncher launcher;
//
//  ExecutionResult createdProgram = launcher.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = launcher.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{Variant(5, TypeIds::Int32), Variant(5, TypeIds::Int32), Variant(5, TypeIds::Int32)};
//  ExecutionResult result = launcher.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage, "mismatched parameters: expected 2 arguments, but got 3");
//}
//
//TEST(VmLauncherDmlfTests, local_none_params)
//{
//  LocalVmLauncher launcher;
//
//  ExecutionResult createdProgram = launcher.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = launcher.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{};
//  ExecutionResult result = launcher.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage, "mismatched parameters: expected 2 arguments, but got 0");
//}
//
//TEST(VmLauncherDmlfTests, local_wrong_type_params)
//{
//  LocalVmLauncher launcher;
//
//  ExecutionResult createdProgram = launcher.CreateExecutable("add", {{"etch", add}});
//  EXPECT_TRUE(createdProgram.succeeded());
//
//
//  ExecutionResult createdState = launcher.CreateState("state");
//  EXPECT_TRUE(createdState.succeeded());
//
//  std::string errorMessage;
//      [&errorMessage](std::string const &, std::string const &, std::string const &,
//                      std::string const &error) { errorMessage = error; });
//
//  Params params{Variant(5, TypeIds::Float32), Variant(5, TypeIds::Int32)};
//  ExecutionResult result = launcher.Run("add",  "state", "add", params);
//
//  EXPECT_FALSE(result.succeeded());
//  EXPECT_EQ(errorMessage,
//            "mismatched parameters: expected argument 0to be of type Int32 but got Float32");
//}

}  // namespace
