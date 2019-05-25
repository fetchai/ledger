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

CustomBindingTests *  fixture;
constexpr std::size_t DEFAULT_TIMES_TO_RUN = 3u;

class CustomBindingTests : public ::testing::Test
{
public:
  void SetUp() override
  {
    fixture = this;
  }

  void compile_and_run_n_times(char const *contract, std::size_t times_to_run)
  {
    ASSERT_TRUE(toolkit.Compile(contract));

    for (auto i = 0u; i < times_to_run; ++i)
    {
      ASSERT_TRUE(toolkit.Run());
    }
  }

  char const *void_no_args = R"(
      function main()
        customBinding();
      endfunction
  )";

  char const *void_with_args = R"(
      function main()
        customBinding(1u32, 2i64);
      endfunction
  )";

  char const *nonvoid_no_args = R"(
      function main()
        var x = Array<Int8>(1);
        x[0] = customBinding();
      endfunction
  )";

  char const *nonvoid_with_args = R"(
      function main()
        var x = Array<UInt16>(1);
        x[0] = customBinding(1u32, 2i64);
      endfunction
  )";

  VmTestToolkit toolkit;

  struct
  {
    MOCK_CONST_METHOD0(increment, void());

    MOCK_CONST_METHOD2(increment_with_args, void(uint32_t, int64_t));
  } call_counter;
};

void CustomBinding_void_no_args(fetch::vm::VM *)
{
  fixture->call_counter.increment();
}

void CustomBinding_void_with_args(fetch::vm::VM *, uint32_t a, int64_t b)
{
  fixture->call_counter.increment_with_args(a, b);
}

int8_t CustomBinding_nonvoid_no_args(fetch::vm::VM *)
{
  fixture->call_counter.increment();

  return 42u;
}

uint16_t CustomBinding_nonvoid_with_args(fetch::vm::VM *, uint32_t a, int64_t b)
{
  fixture->call_counter.increment_with_args(a, b);

  return 42u;
}

TEST_F(CustomBindingTests, test_binding_free_function_to_function_pointer_void_no_arguments)
{
  EXPECT_CALL(call_counter, increment()).Times(DEFAULT_TIMES_TO_RUN);

  toolkit.module().CreateFreeFunction("customBinding", &CustomBinding_void_no_args);

  compile_and_run_n_times(void_no_args, DEFAULT_TIMES_TO_RUN);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_function_pointer_void_with_arguments)
{
  EXPECT_CALL(call_counter, increment_with_args(1u, 2)).Times(DEFAULT_TIMES_TO_RUN);

  toolkit.module().CreateFreeFunction("customBinding", &CustomBinding_void_with_args);

  compile_and_run_n_times(void_with_args, DEFAULT_TIMES_TO_RUN);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_function_pointer_nonvoid_no_arguments)
{
  EXPECT_CALL(call_counter, increment()).Times(DEFAULT_TIMES_TO_RUN);

  toolkit.module().CreateFreeFunction("customBinding", &CustomBinding_nonvoid_no_args);

  compile_and_run_n_times(nonvoid_no_args, DEFAULT_TIMES_TO_RUN);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_function_pointer_nonvoid_with_arguments)
{
  EXPECT_CALL(call_counter, increment_with_args(1u, 2)).Times(DEFAULT_TIMES_TO_RUN);

  toolkit.module().CreateFreeFunction("customBinding", &CustomBinding_nonvoid_with_args);

  compile_and_run_n_times(nonvoid_with_args, DEFAULT_TIMES_TO_RUN);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_functor_void_no_arguments)
{
  EXPECT_CALL(call_counter, increment()).Times(DEFAULT_TIMES_TO_RUN);

  auto CustomBinding_lambda = [this](fetch::vm::VM *) { call_counter.increment(); };
  toolkit.module().CreateFreeFunction("customBinding", std::move(CustomBinding_lambda));

  compile_and_run_n_times(void_no_args, DEFAULT_TIMES_TO_RUN);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_functor_void_with_arguments)
{
  EXPECT_CALL(call_counter, increment_with_args(1u, 2)).Times(DEFAULT_TIMES_TO_RUN);

  auto CustomBinding_lambda = [this](fetch::vm::VM *, uint32_t a, int64_t b) {
    call_counter.increment_with_args(a, b);
  };
  toolkit.module().CreateFreeFunction("customBinding", std::move(CustomBinding_lambda));

  compile_and_run_n_times(void_with_args, DEFAULT_TIMES_TO_RUN);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_functor_nonvoid_no_arguments)
{
  EXPECT_CALL(call_counter, increment()).Times(DEFAULT_TIMES_TO_RUN);

  auto CustomBinding_lambda = [this](fetch::vm::VM *) -> int8_t {
    call_counter.increment();

    return 42;
  };
  toolkit.module().CreateFreeFunction("customBinding", std::move(CustomBinding_lambda));

  compile_and_run_n_times(nonvoid_no_args, DEFAULT_TIMES_TO_RUN);
}

TEST_F(CustomBindingTests, test_binding_free_function_to_functor_nonvoid_with_arguments)
{
  EXPECT_CALL(call_counter, increment_with_args(1u, 2)).Times(DEFAULT_TIMES_TO_RUN);

  auto CustomBinding_lambda = [this](fetch::vm::VM *, uint32_t a, int64_t b) -> uint16_t {
    call_counter.increment_with_args(a, b);

    return 42;
  };
  toolkit.module().CreateFreeFunction("customBinding", std::move(CustomBinding_lambda));

  compile_and_run_n_times(nonvoid_with_args, DEFAULT_TIMES_TO_RUN);
}

}  // namespace
