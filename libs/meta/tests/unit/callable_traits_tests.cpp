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

#include "core/macros.hpp"
#include "meta/callable/callable_traits.hpp"
#include "same.hpp"

#include "gmock/gmock.h"

#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace fetch {
namespace meta {

namespace {

using namespace ::testing;

using EmptyArgs = std::tuple<>;
#define ARGS std::size_t const &, int, std::vector<std::string> &
using Args    = std::tuple<ARGS>;
using Void    = void;
using NonVoid = std::string;

#define ASSERTIONS(callable, return_type, args_tuple_type)                                \
  {                                                                                       \
    using Callable = decltype(callable);                                                  \
                                                                                          \
    using Traits = CallableTraits<Callable>;                                              \
    static_assert(Same<return_type, Traits::ReturnType>,                                  \
                  "Test assertion failed: Same<return_type, Traits::ReturnType>");        \
    static_assert(Same<args_tuple_type, Traits::ArgsTupleType>,                           \
                  "Test assertion failed: Same<args_tuple_type, Traits::ArgsTupleType>"); \
                                                                                          \
    ASSERT_EQ(Traits::arg_count(), std::tuple_size<args_tuple_type>::value);              \
                                                                                          \
    auto const callable_is_void = Same<return_type, Void>;                                \
    ASSERT_EQ(Traits::is_void(), callable_is_void);                                       \
  }

// Adds assertions specific to non-static member functions
#define MEMBER_ASSERTIONS(callable, return_type, args_tuple_type, owning_type)   \
  {                                                                              \
    ASSERTIONS(callable, return_type, args_tuple_type);                          \
                                                                                 \
    using ClassType = owning_type;                                               \
    using Callable  = decltype(callable);                                        \
                                                                                 \
    using Traits = CallableTraits<Callable>;                                     \
    static_assert(Same<ClassType, Traits::OwningType>,                           \
                  "Test assertion failed: Same<ClassType, Traits::OwningType>"); \
  }

Void free_function_void_no_args()
{}

NonVoid free_function_nonvoid_no_args()
{
  return {};
}

Void free_function_void(ARGS)
{}

NonVoid free_function_nonvoid(ARGS)
{
  return {};
}

class TestFunctions
{
public:
  // static member functions
  static Void static_member_function_void_no_args()
  {}

  static NonVoid static_member_function_nonvoid_no_args()
  {
    return {};
  }

  static Void static_member_function_void(ARGS)
  {}

  static NonVoid static_member_function_nonvoid(ARGS)
  {
    return {};
  }

  // non-const member functions
  Void nonconst_member_void_no_args()
  {}

  NonVoid nonconst_member_nonvoid_no_args()
  {
    return {};
  }

  Void nonconst_member_void(ARGS)
  {}

  NonVoid nonconst_member_nonvoid(ARGS)
  {
    return {};
  }

  // const member functions
  Void const_member_void_no_args() const
  {}

  NonVoid const_member_nonvoid_no_args() const
  {
    return {};
  }

  Void const_member_void(ARGS) const
  {}

  NonVoid const_member_nonvoid(ARGS) const
  {
    return {};
  }
};

auto functor_void_no_args    = []() -> Void {};
auto functor_nonvoid_no_args = []() -> NonVoid { return {}; };
auto functor_void            = [](ARGS) -> Void {};
auto functor_nonvoid         = [](ARGS) -> NonVoid { return {}; };

auto mutable_functor_void_no_args    = []() mutable -> Void {};
auto mutable_functor_nonvoid_no_args = []() mutable -> NonVoid { return {}; };
auto mutable_functor_void            = [](ARGS) mutable -> Void {};
auto mutable_functor_nonvoid         = [](ARGS) mutable -> NonVoid { return {}; };

struct SimpleFunctor
{
  void operator()(ARGS) const
  {}
};

struct OverloadedFunctor
{
  void operator()()
  {}

  void operator()(ARGS) const
  {}
};

class CallableTraitsTests : public Test
{
public:
  CallableTraitsTests()
  {
    FETCH_UNUSED(free_function_void_no_args);
    FETCH_UNUSED(free_function_nonvoid_no_args);
    FETCH_UNUSED(free_function_void);
    FETCH_UNUSED(free_function_nonvoid);

    FETCH_UNUSED(functor_void_no_args);
    FETCH_UNUSED(functor_nonvoid_no_args);
    FETCH_UNUSED(functor_void);
    FETCH_UNUSED(functor_nonvoid);

    FETCH_UNUSED(mutable_functor_void_no_args);
    FETCH_UNUSED(mutable_functor_nonvoid_no_args);
    FETCH_UNUSED(mutable_functor_void);
    FETCH_UNUSED(mutable_functor_nonvoid);
  }
};

TEST_F(CallableTraitsTests, test_free_function)
{
  ASSERTIONS(free_function_void_no_args, Void, EmptyArgs);
  ASSERTIONS(free_function_nonvoid_no_args, NonVoid, EmptyArgs);
  ASSERTIONS(free_function_void, Void, Args);
  ASSERTIONS(free_function_nonvoid, NonVoid, Args);
}

TEST_F(CallableTraitsTests, test_static_member_function)
{
  ASSERTIONS(&TestFunctions::static_member_function_void_no_args, Void, EmptyArgs);
  ASSERTIONS(&TestFunctions::static_member_function_nonvoid_no_args, NonVoid, EmptyArgs);
  ASSERTIONS(&TestFunctions::static_member_function_void, Void, Args);
  ASSERTIONS(&TestFunctions::static_member_function_nonvoid, NonVoid, Args);
}

TEST_F(CallableTraitsTests, test_nonconst_member_function)
{
  MEMBER_ASSERTIONS(&TestFunctions::nonconst_member_void_no_args, Void, EmptyArgs, TestFunctions);
  MEMBER_ASSERTIONS(&TestFunctions::nonconst_member_nonvoid_no_args, NonVoid, EmptyArgs,
                    TestFunctions);
  MEMBER_ASSERTIONS(&TestFunctions::nonconst_member_void, Void, Args, TestFunctions);
  MEMBER_ASSERTIONS(&TestFunctions::nonconst_member_nonvoid, NonVoid, Args, TestFunctions);
}

TEST_F(CallableTraitsTests, test_const_member_function)
{
  MEMBER_ASSERTIONS(&TestFunctions::const_member_void_no_args, Void, EmptyArgs, TestFunctions);
  MEMBER_ASSERTIONS(&TestFunctions::const_member_nonvoid_no_args, NonVoid, EmptyArgs,
                    TestFunctions);
  MEMBER_ASSERTIONS(&TestFunctions::const_member_void, Void, Args, TestFunctions);
  MEMBER_ASSERTIONS(&TestFunctions::const_member_nonvoid, NonVoid, Args, TestFunctions);
}

TEST_F(CallableTraitsTests, test_functor)
{
  ASSERTIONS(functor_void_no_args, Void, EmptyArgs);
  ASSERTIONS(functor_nonvoid_no_args, NonVoid, EmptyArgs);
  ASSERTIONS(functor_void, Void, Args);
  ASSERTIONS(functor_nonvoid, NonVoid, Args);
}

TEST_F(CallableTraitsTests, test_mutable_functor)
{
  ASSERTIONS(mutable_functor_void_no_args, Void, EmptyArgs);
  ASSERTIONS(mutable_functor_nonvoid_no_args, NonVoid, EmptyArgs);
  ASSERTIONS(mutable_functor_void, Void, Args);
  ASSERTIONS(mutable_functor_nonvoid, NonVoid, Args);
}

TEST_F(CallableTraitsTests, simple_functor_used_directly)
{
  SimpleFunctor functor;
  ASSERTIONS(functor, Void, Args);
}

TEST_F(CallableTraitsTests, simple_functor_operator_used_as_member_function)
{
  MEMBER_ASSERTIONS(&SimpleFunctor::operator(), Void, Args, SimpleFunctor);
}

TEST_F(CallableTraitsTests, functor_with_overloaded_call_operator)
{
  using desired_overload_type = void (OverloadedFunctor::*)(ARGS) const;
  auto desired_overload_pointer =
      static_cast<desired_overload_type>(&OverloadedFunctor::operator());
  MEMBER_ASSERTIONS(desired_overload_pointer, Void, Args, OverloadedFunctor);
}

}  // namespace

}  // namespace meta
}  // namespace fetch
