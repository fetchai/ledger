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

#include "vm/array.hpp"
#include "vm/variant.hpp"
#include "vm_test_toolkit.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <memory>

namespace {

using namespace fetch::vm;

auto const handler = [](VM * /*vm*/, uint8_t /*unused*/, uint16_t /*unused*/) -> bool {
  return true;
};

class CustomType : public Object
{
public:
  CustomType(VM *vm, TypeId type_id, uint8_t /*unused*/, uint16_t /*unused*/)
    : Object{vm, type_id}
  {}
  ~CustomType() override = default;

  static Ptr<CustomType> Constructor(VM *vm, TypeId type_id)
  {
    return Ptr<CustomType>{new CustomType{vm, type_id, 0, 0}};
  }

  static Ptr<CustomType> ConstructorTwoArgs(VM *vm, TypeId type_id, uint8_t x, uint16_t y)
  {
    return Ptr<CustomType>{new CustomType{vm, type_id, x, y}};
  }

  static void AffordableStatic(VM * /*vm*/, TypeId /*type_id*/, uint8_t /*unused*/,
                               uint16_t /*unused*/)
  {}

  static void TooExpensiveStatic(VM * /*vm*/, TypeId /*type_id*/, uint8_t /*unused*/,
                                 uint16_t /*unused*/)
  {}

  void Affordable(uint8_t /*unused*/, uint16_t /*unused*/)
  {}

  void TooExpensive(uint8_t /*unused*/, uint16_t /*unused*/)
  {}

  int16_t GetIndexedValue(AnyInteger const & /*unused*/)
  {
    return 0;
  }

  void SetIndexedValue(AnyInteger const & /*unused*/, int16_t /*unused*/)
  {}
};

ChargeAmount const low_charge_limit  = 10;
ChargeAmount const high_charge_limit = 1000;

ChargeAmount const affordable_charge = 10;
ChargeAmount const expensive_charge  = 1000;

auto affordable_estimator = [](uint8_t x, uint16_t y) -> ChargeAmount {
  return static_cast<ChargeAmount>(low_charge_limit + x * y);
};
auto expensive_estimator = [](uint8_t x, uint16_t y) -> ChargeAmount {
  return static_cast<ChargeAmount>(high_charge_limit + x * y);
};
auto affordable_member_estimator = [](Ptr<CustomType> const & /*this_*/, uint8_t x,
                                      uint16_t y) -> ChargeAmount {
  return static_cast<ChargeAmount>(low_charge_limit + x * y);
};
auto expensive_member_estimator = [](Ptr<CustomType> const & /*this_*/, uint8_t x,
                                     uint16_t y) -> ChargeAmount {
  return static_cast<ChargeAmount>(high_charge_limit + x * y);
};

class VmChargeTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(VmChargeTests, execution_succeeds_when_charge_limit_obeyed)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u8;
      print(u);
      print(', ');

      var pos_i = 42i8;
      print(pos_i);
      print(', ');

      var neg_i = -42i8;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, execution_fails_when_charge_limit_exceeded)
{
  static char const *TEXT = R"(
    function main()
      var u = 42u8;
      print(u);
      print(', ');

      var pos_i = 42i8;
      print(pos_i);
      print(', ');

      var neg_i = -42i8;
      print(neg_i);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module().CreateFreeFunction("soExpensiveItShouldOverflow", handler,
                                      std::numeric_limits<ChargeAmount>::max());

  static char const *TEXT = R"(
    function main()
      soExpensiveItShouldOverflow(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_does_not_overflow_charge_total)
{
  toolkit.module().CreateFreeFunction("tooExpensive", handler, expensive_charge);

  static char const *TEXT = R"(
    function main()
      tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module().CreateFreeFunction("affordable", handler, affordable_charge);

  static char const *TEXT = R"(
    function main()
      affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, ctor_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("TooExpensive")
      .CreateConstructor(&CustomType::ConstructorTwoArgs, expensive_charge);

  static char const *TEXT = R"(
    function main()
      TooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, ctor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("Affordable")
      .CreateConstructor(&CustomType::ConstructorTwoArgs, affordable_charge);

  static char const *TEXT = R"(
    function main()
      Affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, member_function_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .CreateMemberFunction("tooExpensive", &CustomType::TooExpensive, expensive_charge);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj.tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       member_function_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .CreateMemberFunction("affordable", &CustomType::Affordable, affordable_charge);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj.affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       static_member_function_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateStaticMemberFunction("tooExpensive", &CustomType::TooExpensiveStatic,
                                  expensive_charge);

  static char const *TEXT = R"(
    function main()
      CustomType.tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       static_member_function_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateStaticMemberFunction("affordable", &CustomType::AffordableStatic, affordable_charge);

  static char const *TEXT = R"(
    function main()
      CustomType.affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, index_operator_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  auto const getter_charge = expensive_charge;
  auto const setter_charge = expensive_charge;

  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .EnableIndexOperator(&CustomType::GetIndexedValue, &CustomType::SetIndexedValue,
                           getter_charge, setter_charge);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj[3];
      obj[2] = 1i16;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, index_operator_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  auto const getter_charge = affordable_charge;
  auto const setter_charge = affordable_charge;

  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .EnableIndexOperator(&CustomType::GetIndexedValue, &CustomType::SetIndexedValue,
                           getter_charge, setter_charge);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj[3];
      obj[2] = 1i16;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       functor_bind_with_charge_estimate_execution_fails_when_limit_exceeded_with_estimator)
{
  toolkit.module().CreateFreeFunction("tooExpensive", handler, expensive_estimator);

  static char const *TEXT = R"(
    function main()
      tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       functor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed_with_estimator)
{
  toolkit.module().CreateFreeFunction("affordable", handler, affordable_estimator);

  static char const *TEXT = R"(
    function main()
      affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       ctor_bind_with_charge_estimate_execution_fails_when_limit_exceeded_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("TooExpensive")
      .CreateConstructor(&CustomType::ConstructorTwoArgs, expensive_estimator);

  static char const *TEXT = R"(
    function main()
      TooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       ctor_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("Affordable")
      .CreateConstructor(&CustomType::ConstructorTwoArgs, affordable_estimator);

  static char const *TEXT = R"(
    function main()
      Affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       member_function_bind_with_charge_estimate_execution_fails_when_limit_exceeded_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .CreateMemberFunction("tooExpensive", &CustomType::TooExpensive, expensive_member_estimator);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj.tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(
    VmChargeTests,
    member_function_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .CreateMemberFunction("affordable", &CustomType::Affordable, affordable_member_estimator);

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj.affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(
    VmChargeTests,
    static_member_function_bind_with_charge_estimate_execution_fails_when_limit_exceeded_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateStaticMemberFunction("tooExpensive", &CustomType::TooExpensiveStatic,
                                  expensive_estimator);

  static char const *TEXT = R"(
    function main()
      CustomType.tooExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(
    VmChargeTests,
    static_member_function_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateStaticMemberFunction("affordable", &CustomType::AffordableStatic,
                                  affordable_estimator);

  static char const *TEXT = R"(
    function main()
      CustomType.affordable(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       index_operator_bind_with_charge_estimate_execution_fails_when_limit_exceeded_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .EnableIndexOperator(&CustomType::GetIndexedValue, &CustomType::SetIndexedValue,
                           [](Ptr<CustomType> const &, AnyInteger const &) -> ChargeAmount {
                             return expensive_charge;
                           },
                           [](Ptr<CustomType> const &, AnyInteger const &,
                              int16_t const &) -> ChargeAmount { return expensive_charge; });

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj[3];
      obj[2] = 1i16;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       index_operator_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed_with_estimator)
{
  toolkit.module()
      .CreateClassType<CustomType>("CustomType")
      .CreateConstructor(&CustomType::Constructor)
      .EnableIndexOperator(&CustomType::GetIndexedValue, &CustomType::SetIndexedValue,
                           [](Ptr<CustomType> const &, AnyInteger const &) -> ChargeAmount {
                             return affordable_charge;
                           },
                           [](Ptr<CustomType> const &, AnyInteger const &,
                              int16_t const &) -> ChargeAmount { return affordable_charge; });

  static char const *TEXT = R"(
    function main()
      var obj = CustomType();
      obj[3];
      obj[2] = 1i16;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

}  // namespace
