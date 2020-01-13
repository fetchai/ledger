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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/serializers/main_serializer.hpp"
#include "vm/common.hpp"
#include "vm/execution_task.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gtest/gtest.h"

#include <stdexcept>

namespace {

using namespace fetch::vm;
using namespace fetch;
using fetch::vm::ExecutionTask;
using fetch::vm_modules::VMFactory;

void CreateVMAndRunScript(std::string script, ExecutionTask const &task)
{
  fetch::vm::SourceFiles files;
  files.push_back(fetch::vm::SourceFile("hello.etch", std::move(script)));

  auto executable = std::make_unique<Executable>();
  auto module     = VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS);

  // attempt to compile the program
  auto errors = VMFactory::Compile(module, files, *executable);

  // detect compilation errors
  EXPECT_TRUE(errors.empty()) << "Failed to compile:" << std::endl;
  if (!errors.empty())
  {
    for (auto const &line : errors)
    {
      std::cerr << line << '\n';
    }

    return;
  }

  // create the VM instance
  auto vm = std::make_unique<VM>(module.get());
  vm->AttachOutputDevice(VM::STDOUT, std::cout);

  // Unpacking arguments
  ParameterPack params{vm->registered_types()};
  bool const    success_extract = task.DeserializeParameters(vm.get(), params, executable.get(),
                                                          executable->FindFunction(task.function));

  EXPECT_TRUE(success_extract);
  ;
  if (!success_extract)
  {
    return;
  }

  // Execute the requested function
  std::string error;
  Variant     output;

  // Executing
  bool const success_exe = vm->Execute(*executable, task.function, error, output, params);

  // if there is any console output print it
  EXPECT_TRUE(success_exe) << error << std::endl;
}

TEST(ParameterSerialization, NativeCPPTypes)
{
  auto script = R"(
    function myFunction(
      arr: Array<Array<UInt64>>,
      msg: String,
      i: Int64,
      mymap : Map<String, Map<Int64, Int64>>)

      assert(arr.count() == 3);
      assert(arr[0].count() == 4);
      assert(arr[1].count() == 2);
      assert(arr[2].count() == 3);

      assert(arr[0][0] == 9u64);
      assert(arr[0][1] == 2u64);
      assert(arr[0][2] == 3u64);
      assert(arr[0][3] == 4u64);

      assert(arr[1][0] == 2u64);
      assert(arr[1][1] == 3u64);

      assert(arr[2][0] == 2u64);
      assert(arr[2][1] == 3u64);
      assert(arr[2][2] == 4u64);

      assert(msg == "Hello world");

      assert(i == 9183i64);

      assert(mymap.count() == 2);

      var hello = mymap["hello"];
      assert(hello.count() == 2);
      assert(hello[2i64] == 3i64);
      assert(hello[4i64] == 6i64);

      var world = mymap["world"];
      assert(world.count() == 3);
      assert(world[3i64] == 33i64);
      assert(world[6i64] == 66i64);
      assert(world[9i64] == 99i64);

    endfunction
  )";

  // Testing native types
  // Creating execution task
  ExecutionTask task;
  task.function = "myFunction";

  // Creating function arguments
  MsgPackSerializer serializer;

  // Arg1 Array< Array< UInt64 > >
  std::vector<std::vector<uint64_t>> arr{{9, 2, 3, 4}, {2, 3}, {2, 3, 4}};
  serializer << arr << static_cast<std::string>("Hello world") << static_cast<int64_t>(9183);

  std::map<std::string, std::map<int64_t, int64_t>> mymap{{"hello", {{2, 3}, {4, 6}}},
                                                          {"world", {{9, 99}, {6, 66}, {3, 33}}}};

  serializer << mymap;

  // Storing args
  task.parameters = serializer.data();

  // Testing
  CreateVMAndRunScript(script, task);
}

TEST(ParameterSerialization, PairSerialization)
{
  MsgPackSerializer serializer;

  std::pair<int, std::string> pair_in_1;
  pair_in_1.first  = 1;
  pair_in_1.second = "SOMETHING";

  std::pair<std::string, int> pair_in_2;
  pair_in_2.first  = "ELSE";
  pair_in_2.second = -2;

  serializer << pair_in_1;
  serializer << pair_in_2;

  std::pair<int, std::string> pair_out_1;
  std::pair<std::string, int> pair_out_2;

  auto deserializer = MsgPackSerializer{serializer.data()};
  deserializer >> pair_out_1;
  deserializer >> pair_out_2;

  EXPECT_EQ(pair_in_1, pair_out_1);
  EXPECT_EQ(pair_in_2, pair_out_2);
}

TEST(ParameterSerialization, VariantTypes)
{
  auto script = R"(
    function myFunction(
      arr: Array<Array<UInt64>>,
      msg: String,
      i: Int64,
      mymap : Map<String, Int64>)

      assert(arr.count() == 3);
      assert(arr[0].count() == 4);
      assert(arr[1].count() == 2);
      assert(arr[2].count() == 3);

      assert(arr[0][0] == 9u64);
      assert(arr[0][1] == 2u64);
      assert(arr[0][2] == 3u64);
      assert(arr[0][3] == 4u64);

      assert(arr[1][0] == 2u64);
      assert(arr[1][1] == 3u64);

      assert(arr[2][0] == 2u64);
      assert(arr[2][1] == 3u64);
      assert(arr[2][2] == 4u64);

      assert(msg == "Hello world");

      assert(i == 9183i64);

      assert(mymap.count() == 2);
      assert(mymap["hello"]== 2i64);
      assert(mymap["world"]== 29i64);

    endfunction
  )";

  // Creating execution task
  ExecutionTask task;
  task.function = "myFunction";

  // Creating function arguments
  MsgPackSerializer serializer;

  // Arg1 Array< Array< UInt64 > >
  variant::Variant arr = variant::Variant::Array(3);
  arr[0]               = variant::Variant::Array(4);
  arr[1]               = variant::Variant::Array(2);
  arr[2]               = variant::Variant::Array(3);
  arr[0][0]            = static_cast<uint64_t>(9);
  arr[0][1]            = static_cast<uint64_t>(2);
  arr[0][2]            = static_cast<uint64_t>(3);
  arr[0][3]            = static_cast<uint64_t>(4);
  arr[1][0]            = static_cast<uint64_t>(2);
  arr[1][1]            = static_cast<uint64_t>(3);
  arr[2][0]            = static_cast<uint64_t>(2);
  arr[2][1]            = static_cast<uint64_t>(3);
  arr[2][2]            = static_cast<uint64_t>(4);

  serializer << arr << static_cast<std::string>("Hello world") << static_cast<int64_t>(9183);

  variant::Variant mymap = variant::Variant::Object();
  mymap["hello"]         = 2;
  mymap["world"]         = 29;

  serializer << mymap;

  // Storing args
  task.parameters = serializer.data();

  // Testing
  CreateVMAndRunScript(script, task);
}

}  // namespace
