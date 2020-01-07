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

#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

namespace {

using namespace testing;

class EtchFunctionDefinitionAnnotationTests : public Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(EtchFunctionDefinitionAnnotationTests, unannotated_functions_are_permitted)
{
  static char const *TEXT = R"(
    function f1()
    endfunction

    function f2() : String
      return "";
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       functions_annotated_with_init_action_query_problem_work_objective_clear_are_permitted)
{
  static char const *TEXT = R"(
    @init
    function i()
    endfunction

    @action
    function a()
    endfunction

    @query
    function q() : String
      return "";
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction

    @clear
    function c(problem : Int32, solution : Bool)
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       annotations_other_than_init_action_query_problem_work_objective_clear_are_forbidden)
{
  static char const *TEXT = R"(
    @abc
    function f()
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, multiple_annotations_are_forbidden)
{
  static char const *TEXT = R"(
    @query
    @action
    function qa()
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, duplicate_annotations_are_forbidden)
{
  static char const *TEXT = R"(
    @query
    @query
    function q()
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, action_functions_may_return_void_or_Int64)
{
  static char const *TEXT = R"(
    @action
    function a_void()
    endfunction

    @action
    function a_int64() : Int64
      return 0i64;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       action_functions_may_not_return_types_other_than_void_or_Int64)
{
  static char const *TEXT = R"(
    @action
    function a_uint64() : UInt64
      return 0u64;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, query_functions_must_not_be_void)
{
  static char const *TEXT = R"(
    @query
    function q()
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, init_function_may_return_void_or_Int64)
{
  static char const *TEXT1 = R"(
    @init
    function i_void()
    endfunction
  )";

  static char const *TEXT2 = R"(
    @init
    function i_int64() : Int64
      return 0i64;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT1));
  ASSERT_TRUE(toolkit.Compile(TEXT2));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       init_function_may_not_return_types_other_than_void_or_Int64)
{
  static char const *TEXT = R"(
    @init
    function i_uint64() : UInt64
      return 0u64;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, init_function_may_receive_no_arguments)
{
  static char const *TEXT1 = R"(
    @init
    function i_void()
    endfunction
  )";

  static char const *TEXT2 = R"(
    @init
    function i_int64() : Int64
      return 0i64;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT1));
  ASSERT_TRUE(toolkit.Compile(TEXT2));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, init_function_may_receive_one_Address_argument)
{
  static char const *TEXT1 = R"(
    @init
    function i_void(owner : Address)
    endfunction
  )";

  static char const *TEXT2 = R"(
    @init
    function i_int64(owner : Address) : Int64
      return 0i64;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT1));
  ASSERT_TRUE(toolkit.Compile(TEXT2));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       init_function_may_not_receive_arguments_other_than_a_single_Address)
{
  static char const *TEXT1 = R"(
    @init
    function i_void(foo : Int8)
    endfunction
  )";

  static char const *TEXT2 = R"(
    @init
    function i_void(owner : Address, foo : Int8)
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, multiple_init_functions_are_forbidden)
{
  static char const *TEXT = R"(
    @init
    function one() : Int64
      return 0i64;
    endfunction

    @init
    function two() : Int64
      return 0i64;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_permitted_when_all_four_appear_in_one_contract)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       objective_must_receive_two_params_problem_return_type_and_work_return_type)
{
  static char const *TEXT1 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o() : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT2 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT3 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(solution : Bool, problem : Int32) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT4 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool, num : Int16) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
  ASSERT_FALSE(toolkit.Compile(TEXT3));
  ASSERT_FALSE(toolkit.Compile(TEXT4));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       clear_must_receive_two_params_problem_return_type_and_work_return_type)
{
  static char const *TEXT1 = R"(
    @clear
    function c()
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT2 = R"(
    @clear
    function c(problem : Int32)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT3 = R"(
    @clear
    function c(solution : Bool, problem : Int32)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT4 = R"(
    @clear
    function c(problem : Int32, solution : Bool, num : UInt16)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
  ASSERT_FALSE(toolkit.Compile(TEXT3));
  ASSERT_FALSE(toolkit.Compile(TEXT4));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, work_function_must_not_be_void)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256)
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       work_function_must_receive_two_args_the_problem_return_and_uint256)
{
  static char const *TEXT1 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w() : Bool
      return true;
    endfunction
  )";

  static char const *TEXT2 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT3 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(nonce : UInt256, problem : Int32) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT4 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256, num : UInt8) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
  ASSERT_FALSE(toolkit.Compile(TEXT3));
  ASSERT_FALSE(toolkit.Compile(TEXT4));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, problem_function_must_not_be_void)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>)
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       problem_function_must_accept_one_array_of_structured_data)
{
  static char const *TEXT1 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p() : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT2 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : UInt64) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT3 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>, number : UInt64) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT4 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data1 : Array<StructuredData>, data2 : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
  ASSERT_FALSE(toolkit.Compile(TEXT3));
  ASSERT_FALSE(toolkit.Compile(TEXT4));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, clear_function_must_be_void)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool) : UInt64
      return 0u64;
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests, objective_function_must_have_Int64_return_type)
{
  static char const *TEXT1 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool)
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  static char const *TEXT2 = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : UInt64
      return 0u64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_clear_is_missing)
{
  static char const *TEXT = R"(
    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_objective_is_missing)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_problem_is_missing)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_work_is_missing)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_clear_is_duplicated)
{
  static char const *TEXT = R"(
    @clear
    function c1(problem : Int32, solution : Bool)
    endfunction

    @clear
    function c2(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_problem_is_duplicated)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p1(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @problem
    function p2(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_objective_is_duplicated)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o1(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @objective
    function o2(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchFunctionDefinitionAnnotationTests,
       synergetic_annotations_are_forbidden_if_work_is_duplicated)
{
  static char const *TEXT = R"(
    @clear
    function c(problem : Int32, solution : Bool)
    endfunction

    @problem
    function p(data : Array<StructuredData>) : Int32
      return 0i32;
    endfunction

    @objective
    function o(problem : Int32, solution : Bool) : Int64
      return 0i64;
    endfunction

    @work
    function w1(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction

    @work
    function w2(problem : Int32, nonce : UInt256) : Bool
      return true;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

class EtchContractFunctionPrototypeAnnotationTests : public Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(EtchContractFunctionPrototypeAnnotationTests, unannotated_functions_are_forbidden)
{
  static char const *TEXT = R"(
    contract contract_interface
      function foo();
    endcontract
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchContractFunctionPrototypeAnnotationTests, functions_annotated_with_action_are_permitted)
{
  static char const *TEXT = R"(
    contract contract_interface
      @action
      function a();
    endcontract
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchContractFunctionPrototypeAnnotationTests, annotations_other_than_action_are_forbidden)
{
  static char const *TEXT1 = R"(
    contract contract_interface
      @init
      function i();
    endcontract
  )";

  static char const *TEXT2 = R"(
    contract contract_interface
      @problem
      function p(data : Array<StructuredData>) : Int32;
    endcontract
  )";

  static char const *TEXT3 = R"(
    contract contract_interface
      @objective
      function o(problem : Int32, solution : Bool) : Int64;
    endcontract
  )";

  static char const *TEXT4 = R"(
    contract contract_interface
      @work
      function w(problem : Int32, nonce : UInt256);
    endcontract
  )";

  static char const *TEXT5 = R"(
    contract contract_interface
      @clear
      function c(problem : Int32, solution : Bool);
    endcontract
  )";

  static char const *TEXT6 = R"(
    contract contract_interface
      @abc
      function f();
    endcontract
  )";

  static char const *TEXT7 = R"(
    contract contract_interface
      @query
      function q() : Int32;
    endcontract
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
  ASSERT_FALSE(toolkit.Compile(TEXT3));
  ASSERT_FALSE(toolkit.Compile(TEXT4));
  ASSERT_FALSE(toolkit.Compile(TEXT5));
  ASSERT_FALSE(toolkit.Compile(TEXT6));
  ASSERT_FALSE(toolkit.Compile(TEXT7));
}

// TODO(WK) re-enable when we add query support to c2c calls
TEST_F(EtchContractFunctionPrototypeAnnotationTests, DISABLED_multiple_annotations_are_forbidden)
{
  static char const *TEXT = R"(
    contract contract_interface
      @query
      @action
      function qa();
    endcontract
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchContractFunctionPrototypeAnnotationTests, duplicate_annotations_are_forbidden)
{
  static char const *TEXT = R"(
    contract contract_interface
      @action
      @action
      function q();
    endcontract
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(EtchContractFunctionPrototypeAnnotationTests, action_functions_may_return_void_or_Int64)
{
  static char const *TEXT = R"(
    contract contract_interface
      @action
      function a_void();
      @action
      function a_int64() : Int64;
    endcontract
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchContractFunctionPrototypeAnnotationTests,
       action_functions_may_not_return_types_other_than_void_or_Int64)
{
  static char const *TEXT = R"(
    contract contract_interface
      @action
      function a_uint64() : UInt64;
    endcontract
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

// TODO(WK) re-enable when we add query support to c2c calls
TEST_F(EtchContractFunctionPrototypeAnnotationTests, DISABLED_query_functions_must_not_be_void)
{
  static char const *TEXT = R"(
    contract contract_interface
      @query
      function q();
    endcontract
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

class EtchMemberFunctionDefinitionAnnotationTests : public Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(EtchMemberFunctionDefinitionAnnotationTests,
       DISABLED_unannotated_member_functions_are_permitted)
{
  static char const *TEXT = R"(
    struct Clazz
      function foo() : Int16
        return 1i16;
      endfunction
    endstruct
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchMemberFunctionDefinitionAnnotationTests,
       DISABLED_annotated_member_functions_are_forbidden)
{
  static char const *TEXT1 = R"(
    struct Clazz
      @action
      function foo()
      endfunction
    endstruct
  )";

  static char const *TEXT2 = R"(
    struct Clazz
      @init
      function foo()
      endfunction
    endstruct
  )";

  static char const *TEXT3 = R"(
    struct Clazz
      @query
      function foo() : Int16
        return 1i16;
      endfunction
    endstruct
  )";

  static char const *TEXT4 = R"(
    struct Clazz
      @abc
      function foo() : Int16
        return 1i16;
      endfunction
    endstruct
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
  ASSERT_FALSE(toolkit.Compile(TEXT3));
  ASSERT_FALSE(toolkit.Compile(TEXT4));
}

TEST_F(EtchMemberFunctionDefinitionAnnotationTests, DISABLED_unannotated_constructors_are_permitted)
{
  static char const *TEXT = R"(
    struct Clazz
      function Clazz(x : Int16)
      endfunction
    endstruct
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
}

TEST_F(EtchMemberFunctionDefinitionAnnotationTests, DISABLED_annotated_constructors_are_forbidden)
{
  static char const *TEXT1 = R"(
    struct Clazz
      @action
      function Clazz(x : Int16)
      endfunction
    endstruct
  )";

  static char const *TEXT2 = R"(
    struct Clazz
      @init
      function Clazz(x : Int16)
      endfunction
    endstruct
  )";

  static char const *TEXT3 = R"(
    struct Clazz
      @query
      function Clazz(x : Int16)
      endfunction
    endstruct
  )";

  static char const *TEXT4 = R"(
    struct Clazz
      @abc
      function Clazz(x : Int16)
      endfunction
    endstruct
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT1));
  ASSERT_FALSE(toolkit.Compile(TEXT2));
  ASSERT_FALSE(toolkit.Compile(TEXT3));
  ASSERT_FALSE(toolkit.Compile(TEXT4));
}

}  // namespace
