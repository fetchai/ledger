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

template <ChargeAmount operator_charge>
class CustomTypeTemplate : public Object
{
public:
  using SelfType = CustomTypeTemplate<operator_charge>;

  CustomTypeTemplate(VM *vm, TypeId type_id, uint8_t x__, uint16_t y__)
    : Object{vm, type_id}
    , x(x__)
    , y(y__)
  {}
  ~CustomTypeTemplate() override = default;

  static Ptr<SelfType> Constructor(VM *vm, TypeId type_id)
  {
    return Ptr<SelfType>{new SelfType{vm, type_id, 7, 11}};
  }

  static Ptr<SelfType> ConstructorTwoArgs(VM *vm, TypeId type_id, uint8_t x, uint16_t y)
  {
    return Ptr<SelfType>{new SelfType{vm, type_id, x, y}};
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

  void Add(Ptr<Object> &lhso, Ptr<Object> &rhso) override
  {
    Ptr<SelfType> lhs = lhso;
    Ptr<SelfType> rhs = rhso;

    if (IsTemporary())
    {
      lhs->x = static_cast<uint8_t>(lhs->x + rhs->x);
      lhs->y = static_cast<uint16_t>(lhs->y + rhs->y);
    }
    else
    {
      Ptr<SelfType> temp{new SelfType(vm_, vm_->GetTypeId<SelfType>(),
                                      static_cast<uint8_t>(lhs->x + rhs->x),
                                      static_cast<uint16_t>(lhs->y + lhs->y))};
      lhso = std::move(temp);
    }
  }

  ChargeAmount AddChargeEstimator(Ptr<Object> const & /*lhso*/,
                                  Ptr<Object> const & /*rhso*/) override
  {
    return operator_charge;
  }

  void Negate(Ptr<Object> &object) override
  {
    Ptr<SelfType> obj = object;

    if (IsTemporary())
    {
      obj->x = static_cast<uint8_t>(-obj->x);
      obj->y = static_cast<uint16_t>(-obj->y);
    }
    else
    {
      Ptr<SelfType> temp{new SelfType(vm_, vm_->GetTypeId<SelfType>(),
                                      static_cast<uint8_t>(-obj->x),
                                      static_cast<uint16_t>(-obj->y))};
      object = std::move(temp);
    }
  }

  ChargeAmount NegateChargeEstimator(Ptr<Object> const & /*object*/) override
  {
    return operator_charge;
  }

  bool IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<SelfType> lhs = lhso;
    Ptr<SelfType> rhs = rhso;

    return lhs->x == rhs->x && lhs->y == rhs->y;
  }

  ChargeAmount IsEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                      Ptr<Object> const & /*rhso*/) override
  {
    return operator_charge;
  }

  bool IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<SelfType> lhs = lhso;
    Ptr<SelfType> rhs = rhso;

    return lhs->x != rhs->x && lhs->y != rhs->y;
  }

  ChargeAmount IsNotEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                         Ptr<Object> const & /*rhso*/) override
  {
    return operator_charge;
  }

private:
  uint8_t  x;
  uint16_t y;
};

ChargeAmount const low_charge_limit  = 10;
ChargeAmount const high_charge_limit = 1000;

ChargeAmount const affordable_charge = 10;
ChargeAmount const expensive_charge  = 1000;

ChargeAmount const max_charge_amount = std::numeric_limits<ChargeAmount>::max();

using CustomType = CustomTypeTemplate<1>;

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
auto max_charge_estimator = [](uint8_t /*x*/, uint16_t /*y*/) -> ChargeAmount {
  return max_charge_amount;
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

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_does_not_overflow_charge_total)
{
  toolkit.module().CreateFreeFunction("overflowExpensive", handler, max_charge_amount);

  static char const *TEXT = R"(
    function main()
      overflowExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, max_charge_amount));
}

TEST_F(VmChargeTests, functor_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
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
       functor_bind_with_charge_estimate_execution_does_not_overflow_charge_total_with_estimator)
{
  toolkit.module().CreateFreeFunction("overflowExpensive", handler, max_charge_estimator);

  static char const *TEXT = R"(
    function main()
      overflowExpensive(3u8, 4u16);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, max_charge_amount));
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

TEST_F(VmChargeTests, add_operator_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  using AffordableOperatorChargeCustomType = CustomTypeTemplate<affordable_charge>;

  toolkit.module()
      .CreateClassType<AffordableOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&AffordableOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Add);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      var obj3 = obj1 + obj2;

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, add_operator_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  using ExpensiveOperatorChargeCustomType = CustomTypeTemplate<expensive_charge>;

  toolkit.module()
      .CreateClassType<ExpensiveOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&ExpensiveOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Add);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 + obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests, add_operator_bind_with_charge_estimate_execution_fails_when_charge_overflows)
{
  using MaxOperatorChargeCustomType = CustomTypeTemplate<max_charge_amount>;

  toolkit.module()
      .CreateClassType<MaxOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&MaxOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Add);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 + obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, max_charge_amount));
}

TEST_F(VmChargeTests,
       negate_operator_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  using AffordableOperatorChargeCustomType = CustomTypeTemplate<affordable_charge>;

  toolkit.module()
      .CreateClassType<AffordableOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&AffordableOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Negate);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      -obj1;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests, negate_operator_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  using ExpensiveOperatorChargeCustomType = CustomTypeTemplate<expensive_charge>;

  toolkit.module()
      .CreateClassType<ExpensiveOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&ExpensiveOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Negate);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      -obj1;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       negate_operator_bind_with_charge_estimate_execution_fails_when_charge_overflows)
{
  using MaxOperatorChargeCustomType = CustomTypeTemplate<max_charge_amount>;

  toolkit.module()
      .CreateClassType<MaxOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&MaxOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Negate);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      -obj1;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, max_charge_amount));
}

TEST_F(VmChargeTests,
       isequal_operator_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  using AffordableOperatorChargeCustomType = CustomTypeTemplate<affordable_charge>;

  toolkit.module()
      .CreateClassType<AffordableOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&AffordableOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Equal);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 == obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       isequal_operator_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  using ExpensiveOperatorChargeCustomType = CustomTypeTemplate<expensive_charge>;

  toolkit.module()
      .CreateClassType<ExpensiveOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&ExpensiveOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Equal);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 == obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       isequal_operator_bind_with_charge_estimate_execution_fails_when_charge_overflows)
{
  using MaxOperatorChargeCustomType = CustomTypeTemplate<max_charge_amount>;

  toolkit.module()
      .CreateClassType<MaxOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&MaxOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::Equal);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 == obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, max_charge_amount));
}

TEST_F(VmChargeTests,
       isnotequal_operator_bind_with_charge_estimate_execution_succeeds_when_limit_obeyed)
{
  using AffordableOperatorChargeCustomType = CustomTypeTemplate<affordable_charge>;

  toolkit.module()
      .CreateClassType<AffordableOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&AffordableOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::NotEqual);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 != obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run(nullptr, high_charge_limit));
}

TEST_F(VmChargeTests,
       isnotequal_operator_bind_with_charge_estimate_execution_fails_when_limit_exceeded)
{
  using ExpensiveOperatorChargeCustomType = CustomTypeTemplate<expensive_charge>;

  toolkit.module()
      .CreateClassType<ExpensiveOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&ExpensiveOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::NotEqual);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 != obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, low_charge_limit));
}

TEST_F(VmChargeTests,
       isnotequal_operator_bind_with_charge_estimate_execution_fails_when_charge_overflows)
{
  using MaxOperatorChargeCustomType = CustomTypeTemplate<max_charge_amount>;

  toolkit.module()
      .CreateClassType<MaxOperatorChargeCustomType>("CustomType")
      .CreateConstructor(&MaxOperatorChargeCustomType::Constructor)
      .EnableOperator(Operator::NotEqual);

  static char const *TEXT = R"(
    function main()
      var obj1 = CustomType();
      var obj2 = CustomType();
      obj1 != obj2;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run(nullptr, max_charge_amount));
}

}  // namespace
