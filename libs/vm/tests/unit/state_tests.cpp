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

#include "mock_io_observer.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "vm/vm.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/variant.hpp"

#include "gmock/gmock.h"

#include <vector>
#include <string>
#include <memory>
#include <iostream>

namespace {

using testing::_;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromHex;
using fetch::vm::VM;
using fetch::vm::IR;
using fetch::vm::Executable;
using fetch::vm::Compiler;
using fetch::vm::Module;
using fetch::vm::Variant;

using ExecutablePtr = std::unique_ptr<Executable>;
using CompilerPtr   = std::unique_ptr<Compiler>;
using ModulePtr     = std::unique_ptr<Module>;
using IRPtr         = std::unique_ptr<IR>;
using VMPtr         = std::unique_ptr<VM>;
using ObserverPtr   = std::unique_ptr<MockIoObserver>;

class StateTests : public ::testing::Test
{
protected:

  void SetUp() override
  {
    observer_   = std::make_unique<MockIoObserver>();
    module_     = std::make_unique<Module>();
    compiler_   = std::make_unique<Compiler>(module_.get());
    ir_         = std::make_unique<IR>();
    executable_ = std::make_unique<Executable>();
    vm_         = std::make_unique<VM>(module_.get());

    vm_->SetIOObserver(*observer_);
  }

  void TearDown() override
  {
    vm_.reset();
    executable_.reset();
    ir_.reset();
    compiler_.reset();
    module_.reset();
    observer_.reset();
  }

  bool Compile(char const *text)
  {
    std::vector<std::string> errors{};

    // compile the source code
    if (!compiler_->Compile(text, "default", *ir_, errors))
    {
      PrintErrors(errors);
      return false;
    }

    // generate the IR
    if (!vm_->GenerateExecutable(*ir_, "default_ir", *executable_, errors))
    {
      PrintErrors(errors);
      return false;
    }

    return true;
  }

  bool Run()
  {
    vm_->AttachOutputDevice("stdout", std::cout);

    std::string error{};
    Variant output{};
    if (!vm_->Execute(*executable_, "main", error, output))
    {
      std::cout << "Runtime Error: " << error << std::endl;

      return false;
    }

    return true;
  }

  void PrintErrors(std::vector<std::string> const &errors)
  {
    for (auto const &line : errors)
    {
      std::cout << "Compiler Error: " << line << '\n';
    }
    std::cout << std::endl;
  }

  void AddState(std::string const &key, ConstByteArray const &hex_value)
  {
    auto raw_value = FromHex(hex_value);

    observer_->fake_.SetKeyValue(key, raw_value);
  }

  ObserverPtr   observer_;
  ModulePtr     module_;
  CompilerPtr   compiler_;
  IRPtr         ir_;
  ExecutablePtr executable_;
  VMPtr         vm_;
};

TEST_F(StateTests, SanityCheck)
{
  static char const *TEXT = R"(
    function main()
    endfunction
  )";

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, AddressSerializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB");
      var state = State<Address>("addr", data);
      state.set(data);
    endfunction
  )";

  EXPECT_CALL(*observer_, Exists("addr"));
  EXPECT_CALL(*observer_, Write("addr", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, AddressDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB");
      var state = State<Address>("addr", data);
    endfunction
  )";

  AddState("addr", "000000000000000020000000000000002f351e415c71722c379baac9394a947b8a303927b8b8421fb9466ed3db1f5683");

  EXPECT_CALL(*observer_, Exists("addr"));
  EXPECT_CALL(*observer_, Read("addr", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, MapSerializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Map<String, String>();
      var state = State<Map<String, String>>("map", data);
      state.set(data);
  endfunction
  )";

  EXPECT_CALL(*observer_, Exists("map"));
  EXPECT_CALL(*observer_, Write("map", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, MapDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Map<String, String>();
      var state = State<Map<String, String>>("map", data);
  endfunction
  )";

  AddState("map", "0000000000000000");

  EXPECT_CALL(*observer_, Exists("map"));
  EXPECT_CALL(*observer_, Read("map", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, ArraySerializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Float32>(10);
      var state = State<Array<Float32>>("state", Array<Float32>(0));
      state.set(data);
    endfunction
  )";

  EXPECT_CALL(*observer_, Exists("state"));
  EXPECT_CALL(*observer_, Write("state", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, ArrayDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Float32>(10);
      var state = State<Array<Float32>>("state", Array<Float32>(0));
    endfunction
  )";

  AddState("state", "0c000a0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");

  EXPECT_CALL(*observer_, Exists("state"));
  EXPECT_CALL(*observer_, Read("state", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

} // namespace 

