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

#include "meta/callable/apply.hpp"
#include "meta/callable/invoke.hpp"

#include "gmock/gmock.h"

#include <string>

using namespace ::testing;

namespace fetch {
namespace meta {

namespace {

enum class FunctionReturn
{
  base_non_pure_virtual,
  derived_non_pure_virtual,
  base_non_pure_virtual_const,
  derived_non_pure_virtual_const,
  non_virtual,
  function_ptr,
  functor
};

class Base
{
public:
  virtual FunctionReturn non_pure_virtual()
  {
    return FunctionReturn::base_non_pure_virtual;
  }

  virtual FunctionReturn non_pure_virtual_const() const
  {
    return FunctionReturn::base_non_pure_virtual_const;
  }

  FunctionReturn non_virtual()
  {
    return FunctionReturn::non_virtual;
  }
};

class Derived : public Base
{
public:
  FunctionReturn non_pure_virtual() override
  {
    return FunctionReturn::derived_non_pure_virtual;
  }

  FunctionReturn non_pure_virtual_const() const override
  {
    return FunctionReturn::derived_non_pure_virtual_const;
  }
};

auto functor = []() -> FunctionReturn { return FunctionReturn::functor; };

FunctionReturn function()
{
  return FunctionReturn::function_ptr;
}

class ApplyInvokeMetaTests : public Test
{
public:
  Base    base;
  Derived derived;
  Base *  derived_via_base_ptr = &derived;
  Base &  derived_via_base_ref = derived;
};

TEST_F(ApplyInvokeMetaTests, Apply_non_pure_virtual_member_function)
{
  auto const actual1 = meta::Apply(&Base::non_pure_virtual, &base, std::tuple<>{});
  ASSERT_EQ(actual1, FunctionReturn::base_non_pure_virtual);

  auto const actual2 = meta::Apply(&Derived::non_pure_virtual, &derived, std::tuple<>{});
  ASSERT_EQ(actual2, FunctionReturn::derived_non_pure_virtual);

  auto const actual3 = meta::Apply(&Base::non_pure_virtual, derived_via_base_ptr, std::tuple<>{});
  ASSERT_EQ(actual3, FunctionReturn::derived_non_pure_virtual);

  auto const actual4 = meta::Apply(&Base::non_pure_virtual, derived_via_base_ref, std::tuple<>{});
  ASSERT_EQ(actual4, FunctionReturn::derived_non_pure_virtual);

  auto const actual5 = meta::Apply(&Base::non_pure_virtual, &derived, std::tuple<>{});
  ASSERT_EQ(actual5, FunctionReturn::derived_non_pure_virtual);
}

TEST_F(ApplyInvokeMetaTests, Invoke_non_pure_virtual_member_function)
{
  auto const actual1 = meta::Invoke(&Base::non_pure_virtual, &base);
  ASSERT_EQ(actual1, FunctionReturn::base_non_pure_virtual);

  auto const actual2 = meta::Invoke(&Derived::non_pure_virtual, &derived);
  ASSERT_EQ(actual2, FunctionReturn::derived_non_pure_virtual);

  auto const actual3 = meta::Invoke(&Base::non_pure_virtual, derived_via_base_ptr);
  ASSERT_EQ(actual3, FunctionReturn::derived_non_pure_virtual);

  auto const actual4 = meta::Invoke(&Base::non_pure_virtual, derived_via_base_ref);
  ASSERT_EQ(actual4, FunctionReturn::derived_non_pure_virtual);

  auto const actual5 = meta::Invoke(&Base::non_pure_virtual, &derived);
  ASSERT_EQ(actual5, FunctionReturn::derived_non_pure_virtual);
}

TEST_F(ApplyInvokeMetaTests, Apply_non_pure_virtual_const_member_function)
{
  auto const actual1 = meta::Apply(&Base::non_pure_virtual_const, &base, std::tuple<>{});
  ASSERT_EQ(actual1, FunctionReturn::base_non_pure_virtual_const);

  auto const actual2 = meta::Apply(&Derived::non_pure_virtual_const, &derived, std::tuple<>{});
  ASSERT_EQ(actual2, FunctionReturn::derived_non_pure_virtual_const);

  auto const actual3 =
      meta::Apply(&Base::non_pure_virtual_const, derived_via_base_ptr, std::tuple<>{});
  ASSERT_EQ(actual3, FunctionReturn::derived_non_pure_virtual_const);

  auto const actual4 =
      meta::Apply(&Base::non_pure_virtual_const, derived_via_base_ref, std::tuple<>{});
  ASSERT_EQ(actual4, FunctionReturn::derived_non_pure_virtual_const);

  auto const actual5 = meta::Apply(&Base::non_pure_virtual_const, &derived, std::tuple<>{});
  ASSERT_EQ(actual5, FunctionReturn::derived_non_pure_virtual_const);
}

TEST_F(ApplyInvokeMetaTests, Invoke_non_pure_virtual_const_member_function)
{
  auto const actual1 = meta::Invoke(&Base::non_pure_virtual_const, &base);
  ASSERT_EQ(actual1, FunctionReturn::base_non_pure_virtual_const);

  auto const actual2 = meta::Invoke(&Derived::non_pure_virtual_const, &derived);
  ASSERT_EQ(actual2, FunctionReturn::derived_non_pure_virtual_const);

  auto const actual3 = meta::Invoke(&Base::non_pure_virtual_const, derived_via_base_ptr);
  ASSERT_EQ(actual3, FunctionReturn::derived_non_pure_virtual_const);

  auto const actual4 = meta::Invoke(&Base::non_pure_virtual_const, derived_via_base_ref);
  ASSERT_EQ(actual4, FunctionReturn::derived_non_pure_virtual_const);

  auto const actual5 = meta::Invoke(&Base::non_pure_virtual_const, &derived);
  ASSERT_EQ(actual5, FunctionReturn::derived_non_pure_virtual_const);
}

TEST_F(ApplyInvokeMetaTests, Apply_non_virtual_non_static_member_function)
{
  auto const actual_base = meta::Apply(&Base::non_virtual, &base, std::tuple<>{});
  ASSERT_EQ(actual_base, FunctionReturn::non_virtual);

  auto const actual_inherited = meta::Apply(&Derived::non_virtual, &derived, std::tuple<>{});
  ASSERT_EQ(actual_inherited, FunctionReturn::non_virtual);
}

TEST_F(ApplyInvokeMetaTests, Invoke_non_virtual_non_static_member_function)
{
  auto const result_base = meta::Invoke(&Base::non_virtual, &base);
  ASSERT_EQ(result_base, FunctionReturn::non_virtual);

  auto const result_inherited = meta::Invoke(&Derived::non_virtual, &derived);
  ASSERT_EQ(result_inherited, FunctionReturn::non_virtual);
}

TEST_F(ApplyInvokeMetaTests, Apply_functor)
{
  auto const result = meta::Apply(functor, std::tuple<>{});
  ASSERT_EQ(result, FunctionReturn::functor);
}

TEST_F(ApplyInvokeMetaTests, Invoke_functor)
{
  auto const result = meta::Invoke(functor);
  ASSERT_EQ(result, FunctionReturn::functor);
}

TEST_F(ApplyInvokeMetaTests, Apply_function)
{
  auto const result = meta::Apply(function, std::tuple<>{});
  ASSERT_EQ(result, FunctionReturn::function_ptr);
}

TEST_F(ApplyInvokeMetaTests, Invoke_function)
{
  auto const result = meta::Invoke(function);
  ASSERT_EQ(result, FunctionReturn::function_ptr);
}

}  // namespace

}  // namespace meta
}  // namespace fetch
