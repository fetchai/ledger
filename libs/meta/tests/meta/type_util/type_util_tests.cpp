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

#include "meta/type_util.hpp"

#include "gtest/gtest.h"

#include <tuple>
#include <type_traits>

// googletest cannot into commas in assertions so we'll need few more macros defined.
#define ASSERT_HOLDS(...)               \
  {                                     \
    auto value = (__VA_ARGS__);         \
    ASSERT_TRUE(value) << #__VA_ARGS__; \
  }

#define ASSERT_UNTRUE(...) ASSERT_HOLDS(!(__VA_ARGS__))

// And since we're defining extra assertion macros why not add a few more specialized.
#define ASSERT_TYPE_EQ(...) ASSERT_HOLDS(std::is_same<__VA_ARGS__>::value)

TEST(TypeUtilTests, Constants)
{
  ASSERT_TYPE_EQ(fetch::type_util::TypeConstant<int>::type, int);
  ASSERT_TYPE_EQ(fetch::type_util::TypeConstantT<int>, int);
}

TEST(TypeUtilTests, MemberTypes)
{
  struct A
  {
    using type = struct
    {
    };
  };

  struct B
  {
    using type = int;
  };

  struct C
  {
  };

  ASSERT_TRUE(fetch::type_util::HasMemberTypeV<A>);
  ASSERT_TRUE(fetch::type_util::HasMemberTypeV<B>);
  ASSERT_TYPE_EQ(fetch::type_util::MemberTypeT<B>, int);
  ASSERT_FALSE(fetch::type_util::HasMemberTypeV<C>);
}

class NonConstructible
{
public:
  NonConstructible(bool);
  void M(char, int, NonConstructible, double);
  void CM(char, int, NonConstructible, double) const;
};

TEST(TypeUtilTests, BasicListInterface)
{
  ASSERT_TYPE_EQ(fetch::type_util::HeadT<char, int, NonConstructible, double>, char);
  ASSERT_TYPE_EQ(fetch::type_util::LastT<char, int, NonConstructible, double>, double);

  ASSERT_TYPE_EQ(fetch::type_util::HeadArgumentT<
                     std::is_constructible<NonConstructible, char, int, NonConstructible, double>>,
                 NonConstructible);
  ASSERT_TYPE_EQ(fetch::type_util::HeadArgumentT<decltype(&NonConstructible::M)>, char);
  ASSERT_TYPE_EQ(fetch::type_util::HeadArgumentT<decltype(&NonConstructible::CM)>, char);
}

template <class T, class... Args>
using F = std::is_constructible<T, Args...>;

TEST(TypeUtilTests, ScalarFunctionals)
{
  ASSERT_TYPE_EQ(
      fetch::type_util::AccumulateT<fetch::pack::Pack, char, int, NonConstructible, double>,
      fetch::pack::Pack<fetch::pack::Pack<fetch::pack::Pack<char, int>, NonConstructible>, double>);

  ASSERT_UNTRUE(fetch::type_util::AndV<std::false_type, std::false_type>);
  ASSERT_UNTRUE(fetch::type_util::AndV<std::true_type, std::false_type>);
  ASSERT_UNTRUE(fetch::type_util::AndV<std::false_type, std::true_type>);
  ASSERT_HOLDS(fetch::type_util::AndV<std::true_type, std::true_type>);

  ASSERT_UNTRUE(fetch::type_util::OrV<std::false_type, std::false_type>);
  ASSERT_HOLDS(fetch::type_util::OrV<std::true_type, std::false_type>);
  ASSERT_HOLDS(fetch::type_util::OrV<std::false_type, std::true_type>);
  ASSERT_HOLDS(fetch::type_util::OrV<std::true_type, std::true_type>);

  using T = std::true_type;
  using F = std::false_type;

  ASSERT_HOLDS(fetch::type_util::ConjunctionV<T, T, T, T>);
  ASSERT_UNTRUE(fetch::type_util::ConjunctionV<T, F, T, T>);
  ASSERT_UNTRUE(fetch::type_util::ConjunctionV<T, T, T, F>);

  ASSERT_HOLDS(fetch::type_util::DisjunctionV<T, T, T, T>);
  ASSERT_HOLDS(fetch::type_util::DisjunctionV<T, F, T, T>);
  ASSERT_HOLDS(fetch::type_util::DisjunctionV<T, T, T, F>);
  ASSERT_HOLDS(fetch::type_util::DisjunctionV<F, F, T, F>);
  ASSERT_HOLDS(fetch::type_util::DisjunctionV<F, F, F, T>);
  ASSERT_UNTRUE(fetch::type_util::DisjunctionV<F, F, F, F>);

  ASSERT_HOLDS(fetch::type_util::AllV<std::is_constructible, char, int, double>);
  ASSERT_UNTRUE(fetch::type_util::AllV<std::is_constructible, char, int, NonConstructible, double>);

  ASSERT_HOLDS(fetch::type_util::AnyV<std::is_constructible, char, int, double>);
  ASSERT_HOLDS(fetch::type_util::AnyV<std::is_constructible, char, int, NonConstructible, double>);
  ASSERT_UNTRUE(fetch::type_util::AnyV<std::is_constructible, NonConstructible, NonConstructible,
                                       NonConstructible>);

  ASSERT_HOLDS(fetch::type_util::IsAnyOfV<char, char, int, NonConstructible, double>);
  ASSERT_HOLDS(fetch::type_util::IsAnyOfV<int, char, int, NonConstructible, double>);
  ASSERT_HOLDS(fetch::type_util::IsAnyOfV<NonConstructible, char, int, NonConstructible, double>);
  ASSERT_HOLDS(fetch::type_util::IsAnyOfV<double, char, int, NonConstructible, double>);
  ASSERT_UNTRUE(fetch::type_util::IsAnyOfV<std::true_type, char, int, NonConstructible, double>);

  ASSERT_HOLDS(fetch::type_util::SatisfiesV<double, std::is_constructible, std::is_scalar,
                                            std::is_arithmetic>);
  ASSERT_UNTRUE(fetch::type_util::SatisfiesV<double, std::is_constructible, std::is_scalar,
                                             std::is_arithmetic, std::is_integral>);
}

template <class... Ts>
using Ctor = fetch::type_util::Bind<fetch::pack::Pack, char, int>::template type<Ts...>;

inline void f(char, int, NonConstructible, double);
inline int  g(char, int, NonConstructible);

TEST(TypeUtilTests, ScalarUtils)
{
  ASSERT_TYPE_EQ(Ctor<NonConstructible, double>,
                 fetch::pack::Pack<char, int, NonConstructible, double>);
  ASSERT_UNTRUE(fetch::type_util::IsInvocableV<decltype(&f)>);
  ASSERT_UNTRUE(fetch::type_util::IsInvocableV<decltype(&f), char>);
  ASSERT_UNTRUE(fetch::type_util::IsInvocableV<decltype(&f), char, int>);
  ASSERT_UNTRUE(fetch::type_util::IsInvocableV<decltype(&f), char, int, NonConstructible>);
  ASSERT_HOLDS(fetch::type_util::IsInvocableV<decltype(&f), char, int, NonConstructible, double>);

  ASSERT_TYPE_EQ(fetch::type_util::InvokeResultT<decltype(&f), char, int, NonConstructible, double>,
                 void);
  ASSERT_TYPE_EQ(fetch::type_util::InvokeResultT<decltype(&g), char, int, NonConstructible>, int);

  ASSERT_EQ(fetch::type_util::detail_::ReturnZero<int>::Call(), 0);
  ASSERT_FALSE(fetch::type_util::detail_::ReturnZero<bool>::Call());
  ASSERT_TYPE_EQ(decltype(fetch::type_util::detail_::ReturnZero<void>::Call()), void);

  ASSERT_TYPE_EQ(
      fetch::type_util::LiftIntegerSequenceT<std::index_sequence<0, 3, 42>>,
      fetch::pack::Pack<std::index_sequence<0>, std::index_sequence<3>, std::index_sequence<42>>);

  ASSERT_TYPE_EQ(fetch::type_util::CopyReferenceKindT<int, char>, char);
  ASSERT_TYPE_EQ(fetch::type_util::CopyReferenceKindT<int &, char>, char &);
  ASSERT_TYPE_EQ(fetch::type_util::CopyReferenceKindT<int &&, char>, char &&);
}

TEST(TypeUtilTests, SwitchesAndSelects)
{
  ASSERT_TYPE_EQ(
      fetch::type_util::SwitchT<std::false_type, char, std::false_type, int, std::true_type,
                                NonConstructible, std::false_type, double>,
      NonConstructible);
  ASSERT_TYPE_EQ(
      fetch::type_util::SwitchT<std::false_type, char, std::false_type, int, std::false_type,
                                NonConstructible, std::false_type, double, float>,
      float);

  struct A
  {
  };
  struct B
  {
  };
  struct C
  {
    using type = int;
  };
  struct D
  {
  };

  ASSERT_TYPE_EQ(fetch::type_util::SelectT<A, B, C, D>, int);
}

using Values = std::tuple<char, int, double>;

template <std::size_t index>
class Setter : public fetch::pack::SizeConstant<index>
{
  Values &values;

public:
  using fetch::pack::SizeConstant<index>::value;

  Setter(Values &values)
    : values(values)
  {}

  constexpr auto &get() const
  {
    return std::get<value>(values);
  }
};

TEST(TypeUtilTests, BinarySwitch)
{
  using BS = fetch::type_util::BinarySwitch<Setter<0>, Setter<1>, Setter<2>>;

  Values v{};

  ASSERT_HOLDS(BS::Call(fetch::pack::SizeConstant<0>{},
                        [](auto &&x) {
                          x.get() = 'H';
                          return 0;
                        },
                        v) == 0);
  ASSERT_HOLDS(BS::Call(fetch::pack::SizeConstant<1>{},
                        [](auto &&x) {
                          x.get() = 42;
                          return 1;
                        },
                        v) == 1);
  ASSERT_HOLDS(BS::Call(fetch::pack::SizeConstant<2>{},
                        [](auto &&x) {
                          x.get() = 3;
                          return 2;
                        },
                        v) == 2);

  // try to match an index unknown to this switch
  ASSERT_HOLDS(BS::Call(fetch::pack::SizeConstant<3>{},
                        [](auto &&x) {
                          x.get() = 14;
                          return 42;
                        },
                        v) == 0);

  ASSERT_HOLDS(v == Values{'H', 42, 3});
}
