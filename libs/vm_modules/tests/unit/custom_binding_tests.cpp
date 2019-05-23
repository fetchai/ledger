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
#include <cstdint>

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

  char const *void_two_args =
      " function main() "
      "   customBinding(1u32, 2i64);"
      " endfunction ";

  VmTestToolkit toolkit;
  struct
  {
    MOCK_CONST_METHOD0(increment, void());
    MOCK_CONST_METHOD2(increment_with_args, void(uint32_t, int64_t));
  } call_counter;
};

void CustomBinding_function_no_args(fetch::vm::VM *)
{
  fixture->call_counter.increment();
}

void CustomBinding_function_two_args(fetch::vm::VM *, uint32_t a, int64_t b)
{
  fixture->call_counter.increment_with_args(a, b);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_function_pointer_void_no_arguments)
{
  EXPECT_CALL(call_counter, increment()).Times(3);

  toolkit.module().CreateFreeFunction("customBinding", &CustomBinding_function_no_args);

  ASSERT_TRUE(toolkit.Compile(void_no_args));

  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(CustomBindingTests, test_binding_free_function_to_function_pointer_void_with_arguments)
{
  EXPECT_CALL(call_counter, increment_with_args(1u, 2)).Times(3);

  toolkit.module().CreateFreeFunction("customBinding", &CustomBinding_function_two_args);

  ASSERT_TRUE(toolkit.Compile(void_two_args));

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

TEST_F(CustomBindingTests, test_binding_free_function_to_functor_void_with_arguments)
{
  EXPECT_CALL(call_counter, increment_with_args(1u, 2)).Times(3);

  auto CustomBinding_lambda = [this](fetch::vm::VM *, uint32_t a, int64_t b) {
    call_counter.increment_with_args(a, b);
  };
  toolkit.module().CreateFreeFunction("customBinding", CustomBinding_lambda);

  ASSERT_TRUE(toolkit.Compile(void_two_args));

  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
  ASSERT_TRUE(toolkit.Run());
}

}  // namespace
