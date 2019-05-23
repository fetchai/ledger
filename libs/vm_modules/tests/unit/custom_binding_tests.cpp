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

#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <cstddef>

namespace {

class CustomBindingTests;

CustomBindingTests *fixture;

class CustomBindingTests : public ::testing::Test
{
public:
  void SetUp() override
  {
    fixture = this;
  }

  char const *void_no_args =
      " function main() "
      "   customBinding();"
      " endfunction ";

  VmTestToolkit toolkit;
  struct
  {
    MOCK_CONST_METHOD0(increment, void());
  } call_counter;
};

void CustomBinding_function(fetch::vm::VM *)
{
  fixture->call_counter.increment();
}

TEST_F(CustomBindingTests, test_binding_free_function_to_function_pointer_void_no_arguments)
{
  EXPECT_CALL(call_counter, increment()).Times(3);

  toolkit.module().CreateFreeFunction("customBinding", &CustomBinding_function);

  ASSERT_TRUE(toolkit.Compile(void_no_args));

  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(CustomBindingTests, test_binding_free_function_to_functor_void_no_arguments)
{
  EXPECT_CALL(call_counter, increment()).Times(3);

  auto CustomBinding_lambda = [this](fetch::vm::VM *) { call_counter.increment(); };
  toolkit.module().CreateFreeFunction("customBinding", CustomBinding_lambda);

  ASSERT_TRUE(toolkit.Compile(void_no_args));

  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
}

}  // namespace
