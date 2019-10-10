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
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

namespace {

namespace {

using fetch::vm_modules::VMFactory;
using namespace fetch::vm;
using namespace fetch::dmlf;

using Params = fetch::dmlf::LocalVmLauncher::Params;
//using Status = fetch::dmlf::VmWrapperInterface::Status;

auto const helloWorld = R"(
function main()

  printLn("Hello world!!");

endfunction)";

auto const tick = R"(

persistent tick : Int32;

function main()

  use tick;

  printLn(toString(tick.get(0)));

  tick.set(tick.get(0) + 1);

endfunction
)";

auto const tick2 = R"(

persistent tick : Int32;

function tick2()

  use tick;

  printLn(toString(tick.get(0)));

  tick.set(tick.get(0) + 2);

endfunction
)";

auto const tock = R"(

persistent tock : Int32;

function tock()

  use tock;

  printLn(toString(tock.get(0)));

  tock.set(tock.get(0) + 1);

endfunction
)";

auto const tickTock = R"(

persistent tick : Int32;
persistent tock : Int32;

function tick()

  use tick;

  printLn(toString(tick.get(0)));

  tick.set(tick.get(0) + 1);

endfunction

function tock()

  use tock;

  printLn(toString(tock.get(0)));

  tock.set(tock.get(0) + 2);

endfunction
)";

auto programOut = [] (std::string name, std::vector<std::string> const& out) 
{
  std::cout << "Error making program " << name << '\n';
  for (auto const& l : out)
    std::cout << l << '\n';
};

auto executeOut = [] ( std::string const& program , std::string const& vm, std::string const& state, std::string const& error)
{
  std::cout << "Error running " << program << " on " << vm << " from state " << state << '\n';
  std::cout << '\t' << error << '\n';
};

}  // namespace

TEST(VmLauncherDmlfTests, local_HelloWorld)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);
  EXPECT_FALSE(launcher.HasProgram("helloWorld"));
  EXPECT_FALSE(launcher.HasVM("vm"));
  EXPECT_FALSE(launcher.HasVM("state"));

  bool createdProgram = launcher.CreateProgram("helloWorld", helloWorld);
  EXPECT_TRUE(createdProgram);

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  std::stringstream output;
  launcher.SetVmStdout("vm", output);

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("helloWorld", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "Hello world!!\n");
}

TEST(VmLauncherDmlfTests, local_DoubleHelloWorld)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);
  EXPECT_FALSE(launcher.HasProgram("helloWorld"));
  EXPECT_FALSE(launcher.HasVM("vm"));
  EXPECT_FALSE(launcher.HasVM("state"));

  bool createdProgram = launcher.CreateProgram("helloWorld", helloWorld);
  EXPECT_TRUE(createdProgram);

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  std::stringstream output;
  launcher.SetVmStdout("vm", output);

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("helloWorld", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("helloWorld", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "Hello world!!\nHello world!!\n");
}

TEST(VmLauncherDmlfTests, repeated_HelloWorld)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);
  EXPECT_FALSE(launcher.HasProgram("helloWorld"));
  EXPECT_FALSE(launcher.HasVM("vm"));
  EXPECT_FALSE(launcher.HasVM("state"));

  bool createdProgram = launcher.CreateProgram("helloWorld", helloWorld);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("helloWorld"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  launcher.SetVmStdout("vm", output);

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  createdProgram = launcher.CreateProgram("helloWorld", helloWorld);
  createdVM = launcher.CreateVM("vm");
  createdState = launcher.CreateState("state");

  EXPECT_FALSE(createdProgram);
  EXPECT_FALSE(createdVM);
  EXPECT_FALSE(createdState);

  bool executedSuccesfully = launcher.Execute("helloWorld", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "Hello world!!\n");
}

TEST(VmLauncherDmlfTests, local_Tick_VM_2States)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  std::stringstream output;
  launcher.SetVmStdout("vm", output);

  bool createdState = launcher.CreateState("state1");
  EXPECT_TRUE(createdState);
  createdState = launcher.CreateState("state2");
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state1", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state1", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state2", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm", "state1", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm", "state2", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n1\n0\n2\n1\n");
}

// Repeated things

// Breaks VM
//TEST(VmLauncherDmlfTests, break_vm)
//{
//  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);
//
//  bool createdVM = launcher.CreateVM("vm");
//  EXPECT_TRUE(createdVM);
//  EXPECT_TRUE(launcher.HasVM("vm"));
//  std::stringstream output;
//  bool setStdOut = launcher.SetVmStdout("vm", output);
//  EXPECT_TRUE(setStdOut);
//
//  bool createdProgram = launcher.CreateProgram("helloWorld", helloWorld);
//  EXPECT_TRUE(createdProgram);
//  EXPECT_TRUE(launcher.HasProgram("helloWorld"));
//
//
//  bool createdState = launcher.CreateState("state");
//  EXPECT_TRUE(launcher.HasState("state"));
//  EXPECT_TRUE(createdState);
//
//  bool executedSuccesfully = launcher.Execute("helloWorld", "vm", "state", "main", Params());
//  EXPECT_TRUE(executedSuccesfully);
//
//  EXPECT_EQ(output.str(), "Hello world!!\n");
//}

TEST(VmLauncherDmlfTests, bad_stdOut)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("helloWorld", helloWorld);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("helloWorld"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);
  std::stringstream badOutput;
  bool badStdOut = launcher.SetVmStdout("badVm", badOutput);
  EXPECT_FALSE(badStdOut);

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("helloWorld", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "Hello world!!\n");
}

TEST(VmLauncherDmlfTests, local_Tick_Tick2_VM_State)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);

  createdProgram = launcher.CreateProgram("tick2", tick2);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick2"));

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n1\n3\n4\n6\n7\n9\n10\n");
}

TEST(VmLauncherDmlfTests, test_Tick_Tock_VM_State)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);

  createdProgram = launcher.CreateProgram("tock", tock);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tock"));

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n0\n1\n1\n2\n2\n3\n3\n");
}

TEST(VmLauncherDmlfTests, test_Tick_TickTock_VM_State)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);

  createdProgram = launcher.CreateProgram("tickTock", tickTock);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tickTock"));

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tick", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);


  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tick", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tick", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n1\n0\n2\n3\n2\n4\n5\n4\n");
}

TEST(VmLauncherDmlfTests, test_TickState_TockState2_VM)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);

  createdProgram = launcher.CreateProgram("tick2", tick2);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick2"));

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);
  createdState = launcher.CreateState("state2");
  EXPECT_TRUE(launcher.HasState("state2"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state2", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state2", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state2", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state2", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "tick2", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n0\n1\n2\n4\n2\n5\n4\n");
}

TEST(VmLauncherDmlfTests, test_Tick_Tock_TickTock_VM_State)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);

  createdProgram = launcher.CreateProgram("tickTock", tickTock);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tickTock"));

  createdProgram = launcher.CreateProgram("tock", tock);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tock"));

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tick", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);


  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tick", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tickTock", "vm", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n0\n1\n1\n2\n3\n3\n4\n");
}

TEST(VmLauncherDmlfTests, test_HelloWorld_2VM)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("helloWorld", helloWorld);
  EXPECT_TRUE(createdProgram);

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  std::stringstream output;
  launcher.SetVmStdout("vm", output);
  createdVM = launcher.CreateVM("vm2");
  EXPECT_TRUE(createdVM);
  launcher.SetVmStdout("vm2", output); // Same stream

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("helloWorld", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("helloWorld", "vm2", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "Hello world!!\nHello world!!\n");
}

TEST(VmLauncherDmlfTests, test_Tick_2VM_State)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  std::stringstream output;
  launcher.SetVmStdout("vm", output);
  createdVM = launcher.CreateVM("vm2");
  EXPECT_TRUE(createdVM);
  launcher.SetVmStdout("vm2", output); // Same stream

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm2", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm2", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n1\n2\n3\n");
}

TEST(VmLauncherDmlfTests, test_Tick_Tock_2VM_State)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  createdProgram = launcher.CreateProgram("tock", tock);
  EXPECT_TRUE(createdProgram);

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  std::stringstream output;
  launcher.SetVmStdout("vm", output);
  createdVM = launcher.CreateVM("vm2");
  EXPECT_TRUE(createdVM);
  launcher.SetVmStdout("vm2", output); // Same stream


  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm2", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tock", "vm2", "state", "tock", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n0\n1\n1\n");
}

TEST(VmLauncherDmlfTests, local_Tick_Tick_VM_State)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);

  createdProgram = launcher.CreateProgram("tick2", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick2"));
  createdProgram = launcher.CreateProgram("tick3", tick);
  EXPECT_TRUE(createdProgram);
  createdProgram = launcher.CreateProgram("tick4", tick);
  EXPECT_TRUE(createdProgram);
  createdProgram = launcher.CreateProgram("tick5", tick);
  EXPECT_TRUE(createdProgram);
  createdProgram = launcher.CreateProgram("tick6", tick);
  EXPECT_TRUE(createdProgram);
  createdProgram = launcher.CreateProgram("tick7", tick);
  EXPECT_TRUE(createdProgram);
  createdProgram = launcher.CreateProgram("tick8", tick);
  EXPECT_TRUE(createdProgram);
  createdProgram = launcher.CreateProgram("tick9", tick);
  EXPECT_TRUE(createdProgram);

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick2", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n1\n2\n3\n4\n5\n6\n7\n");
}

TEST(VmLauncherDmlfTests, local_Tick_Tick_VM_CopyState)
{
  LocalVmLauncher launcher; launcher.AttachProgramErrorHandler(programOut); launcher.AttachExecuteErrorHandler(executeOut);

  bool createdProgram = launcher.CreateProgram("tick", tick);
  EXPECT_TRUE(createdProgram);
  EXPECT_TRUE(launcher.HasProgram("tick"));

  bool createdVM = launcher.CreateVM("vm");
  EXPECT_TRUE(createdVM);
  EXPECT_TRUE(launcher.HasVM("vm"));
  std::stringstream output;
  bool setStdOut = launcher.SetVmStdout("vm", output);
  EXPECT_TRUE(setStdOut);

  bool createdState = launcher.CreateState("state");
  EXPECT_TRUE(launcher.HasState("state"));
  EXPECT_TRUE(createdState);

  bool executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  launcher.CopyState("state", "state2");

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm", "state2", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm", "state2", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  executedSuccesfully = launcher.Execute("tick", "vm", "state", "main", Params());
  EXPECT_TRUE(executedSuccesfully);
  executedSuccesfully = launcher.Execute("tick", "vm", "state2", "main", Params());
  EXPECT_TRUE(executedSuccesfully);

  EXPECT_EQ(output.str(), "0\n1\n2\n2\n3\n3\n4\n4\n");
}

}  // namespace
