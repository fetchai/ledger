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

#include "meta/pack.hpp"

#include "gtest/gtest.h"

#include <type_traits>

// googletest cannot into commas in assertions so we'll need few more macros defined.
#define ASSERT_HOLDS(...)               \
{                                       \
  auto value = (__VA_ARGS__);           \
  ASSERT_TRUE(value) << #__VA_ARGS__;   \
}

#define ASSERT_UNTRUE(...) ASSERT_HOLDS(!(__VA_ARGS__))

// And since we're defining extra assertion macros why not add a few more specialized.
#define ASSERT_TYPE_EQ(...) ASSERT_HOLDS(std::is_same<__VA_ARGS__>::value)

TEST(PackTests, Constants) {
	ASSERT_TYPE_EQ(fetch::pack::Constant<int>::type, int);
	ASSERT_TYPE_EQ(fetch::pack::ConstantT<int>, int);

	using S = fetch::pack::SizeConstant<42>;
	static constexpr auto SV = fetch::pack::SizeConstantV<42>;
	ASSERT_EQ(S::value, 42);
	ASSERT_TYPE_EQ(decltype(S::value), const std::size_t);
	ASSERT_EQ(SV, 42);
	ASSERT_TYPE_EQ(decltype(SV), const std::size_t);

	using T = fetch::pack::BoolConstant<true>;
	static constexpr auto TV = fetch::pack::BoolConstantV<true>;
	using F = fetch::pack::BoolConstant<false>;
	static constexpr auto FV = fetch::pack::BoolConstantV<false>;
	ASSERT_TYPE_EQ(decltype(T::value), const bool);
	ASSERT_TYPE_EQ(decltype(TV), const bool)
	ASSERT_TYPE_EQ(decltype(F::value), const bool);
	ASSERT_TYPE_EQ(decltype(FV), const bool);
	ASSERT_TRUE(T::value);
	ASSERT_TRUE(TV);
	ASSERT_FALSE(F::value);
	ASSERT_FALSE(FV);
}

TEST(PackTests, MemberTypes) {
	struct A {
		using type = struct {
			using type = struct {
				using type = int;
			};
		};
	};

	struct B {
		using type = int;
	};

	struct C {};

	ASSERT_TRUE(fetch::pack::HasMemberTypeV<A>);
	ASSERT_TRUE(fetch::pack::HasMemberTypeV<B>);
	ASSERT_TYPE_EQ(fetch::pack::MemberTypeT<B>, int);
	ASSERT_FALSE(fetch::pack::HasMemberTypeV<C>);

	ASSERT_TYPE_EQ(fetch::pack::FlattenT<A>, int);
	ASSERT_TYPE_EQ(fetch::pack::FlatT<B>, B);
	ASSERT_TYPE_EQ(fetch::pack::FlatT<C>::type, C);
}

class NonConstructible {
public:
	NonConstructible(bool);
	void M(char, int, NonConstructible, double);
	void CM(char, int, NonConstructible, double) const;
};

using InputList = fetch::pack::Pack<char, int, NonConstructible, double>;

TEST(PackTests, BasicListInterface) {
	using ConsIntNil = fetch::pack::ConsT<int, fetch::pack::Nil>;
	ASSERT_TYPE_EQ(ConsIntNil, fetch::pack::Singleton<int>);
	ASSERT_TYPE_EQ(fetch::pack::Singleton<int>, fetch::pack::Pack<int>);

	using ConsConsDef = fetch::pack::ConsT<int, fetch::pack::ConsT<double, fetch::pack::Pack<char, float>>>;
	using ConsConsResult = fetch::pack::Pack<int, double, char, float>;
	ASSERT_TYPE_EQ(ConsConsDef, ConsConsResult);

	ASSERT_TYPE_EQ(fetch::pack::HeadT<InputList>, char);
	ASSERT_TYPE_EQ(fetch::pack::TailT<InputList>, fetch::pack::Pack<int, NonConstructible, double>);
	ASSERT_TYPE_EQ(fetch::pack::InitT<InputList>, fetch::pack::Pack<char, int, NonConstructible>);
	ASSERT_TYPE_EQ(fetch::pack::LastT<InputList>, double);

	ASSERT_TYPE_EQ(fetch::pack::ConcatT<fetch::pack::Nil, char, fetch::pack::Pack<int, NonConstructible>, double>, InputList);
	ASSERT_TYPE_EQ(fetch::pack::ConcatT<InputList>, InputList);
}

template<class T, class... Args> using F = std::is_constructible<T, Args...>;

TEST(PackTests, Operators) {
	ASSERT_TYPE_EQ(fetch::pack::TransformT<F, InputList>, fetch::pack::Pack<std::is_constructible<char>, std::is_constructible<int>, std::is_constructible<NonConstructible>, std::is_constructible<double>>);
	ASSERT_TYPE_EQ(fetch::pack::FilterT<F, InputList>, fetch::pack::Pack<char, int, double>);

	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<0>, InputList>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<1>, InputList>, fetch::pack::Pack<char>);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<2>, InputList>, fetch::pack::Pack<char, int>);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<3>, InputList>, fetch::pack::Pack<char, int, NonConstructible>);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<4>, InputList>, InputList);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<5>, InputList>, InputList);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<6>, InputList>, InputList);

	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<0>, fetch::pack::Nil>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<1>, fetch::pack::Nil>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<2>, fetch::pack::Nil>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::TakeT<fetch::pack::SizeConstant<3>, fetch::pack::Nil>, fetch::pack::Nil);

	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<0>, InputList>, InputList);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<1>, InputList>, fetch::pack::Pack<int, NonConstructible, double>);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<2>, InputList>, fetch::pack::Pack<NonConstructible, double>);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<3>, InputList>, fetch::pack::Pack<double>);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<4>, InputList>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<5>, InputList>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<6>, InputList>, fetch::pack::Nil);

	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<0>, fetch::pack::Nil>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<1>, fetch::pack::Nil>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<2>, fetch::pack::Nil>, fetch::pack::Nil);
	ASSERT_TYPE_EQ(fetch::pack::DropT<fetch::pack::SizeConstant<3>, fetch::pack::Nil>, fetch::pack::Nil);

	ASSERT_TYPE_EQ(fetch::pack::LeftHalfT<InputList>, fetch::pack::Pack<char, int>);
	ASSERT_TYPE_EQ(fetch::pack::RightHalfT<InputList>, fetch::pack::Pack<NonConstructible, double>);
}

TEST(PackTests, ScalarFunctionals) {
	ASSERT_TRUE(fetch::pack::EmptyV<fetch::pack::Nil>);
	ASSERT_FALSE(fetch::pack::EmptyV<InputList>);

	ASSERT_EQ(fetch::pack::TupleSizeV<fetch::pack::Nil>, 0);
	ASSERT_EQ(fetch::pack::TupleSizeV<InputList>, 4);

	ASSERT_EQ(std::tuple_size<fetch::pack::Nil>::value, 0);
	ASSERT_EQ(std::tuple_size<InputList>::value, 4);

	ASSERT_TYPE_EQ(fetch::pack::TupleElementT<fetch::pack::SizeConstant<0>, InputList>, char);
	ASSERT_TYPE_EQ(fetch::pack::TupleElementT<fetch::pack::SizeConstant<1>, InputList>, int);
	ASSERT_TYPE_EQ(fetch::pack::TupleElementT<fetch::pack::SizeConstant<2>, InputList>, NonConstructible);
	ASSERT_TYPE_EQ(fetch::pack::TupleElementT<fetch::pack::SizeConstant<3>, InputList>, double);

	ASSERT_TYPE_EQ(std::tuple_element_t<0, InputList>, char);
	ASSERT_TYPE_EQ(std::tuple_element_t<1, InputList>, int);
	ASSERT_TYPE_EQ(std::tuple_element_t<2, InputList>, NonConstructible);
	ASSERT_TYPE_EQ(std::tuple_element_t<3, InputList>, double);

	ASSERT_TYPE_EQ(fetch::pack::AccumulateT<fetch::pack::Pack, InputList>,
		       fetch::pack::Pack<fetch::pack::Pack<fetch::pack::Pack<char, int>, NonConstructible>, double>);

	ASSERT_UNTRUE(fetch::pack::AndV<std::false_type, std::false_type>);
	ASSERT_UNTRUE(fetch::pack::AndV<std::true_type, std::false_type>);
	ASSERT_UNTRUE(fetch::pack::AndV<std::false_type, std::true_type>);
	ASSERT_HOLDS(fetch::pack::AndV<std::true_type, std::true_type>);

	ASSERT_UNTRUE(fetch::pack::OrV<std::false_type, std::false_type>);
	ASSERT_HOLDS(fetch::pack::OrV<std::true_type, std::false_type>);
	ASSERT_HOLDS(fetch::pack::OrV<std::false_type, std::true_type>);
	ASSERT_HOLDS(fetch::pack::OrV<std::true_type, std::true_type>);

	using T = std::true_type;
	using F = std::false_type;

	ASSERT_HOLDS(fetch::pack::ConjunctionV<fetch::pack::Pack<T, T, T, T>>);
	ASSERT_UNTRUE(fetch::pack::ConjunctionV<fetch::pack::Pack<T, F, T, T>>);
	ASSERT_UNTRUE(fetch::pack::ConjunctionV<fetch::pack::Pack<T, T, T, F>>);

	ASSERT_HOLDS(fetch::pack::DisjunctionV<fetch::pack::Pack<T, T, T, T>>);
	ASSERT_HOLDS(fetch::pack::DisjunctionV<fetch::pack::Pack<T, F, T, T>>);
	ASSERT_HOLDS(fetch::pack::DisjunctionV<fetch::pack::Pack<T, T, T, F>>);
	ASSERT_HOLDS(fetch::pack::DisjunctionV<fetch::pack::Pack<F, F, T, F>>);
	ASSERT_HOLDS(fetch::pack::DisjunctionV<fetch::pack::Pack<F, F, F, T>>);
	ASSERT_UNTRUE(fetch::pack::DisjunctionV<fetch::pack::Pack<F, F, F, F>>);

	ASSERT_HOLDS(fetch::pack::AllV<std::is_constructible, fetch::pack::Pack<char, int, double>>);
	ASSERT_UNTRUE(fetch::pack::AllV<std::is_constructible, InputList>);

	ASSERT_HOLDS(fetch::pack::AnyV<std::is_constructible, fetch::pack::Pack<char, int, double>>);
	ASSERT_HOLDS(fetch::pack::AnyV<std::is_constructible, InputList>);
	ASSERT_UNTRUE(fetch::pack::AnyV<std::is_constructible, fetch::pack::Pack<NonConstructible, NonConstructible, NonConstructible>>);

	ASSERT_HOLDS(fetch::pack::IsAnyOfV<char, InputList>);
	ASSERT_HOLDS(fetch::pack::IsAnyOfV<int, InputList>);
	ASSERT_HOLDS(fetch::pack::IsAnyOfV<NonConstructible, InputList>);
	ASSERT_HOLDS(fetch::pack::IsAnyOfV<double, InputList>);
	ASSERT_UNTRUE(fetch::pack::IsAnyOfV<std::true_type, InputList>);
}

template<class... Ts> using Ctor = fetch::pack::Bind<fetch::pack::Pack, fetch::pack::Pack<char, int>>::template type<Ts...>;

inline void f(char, int, NonConstructible, double);
inline int g(char, int, NonConstructible);

TEST(PackTests, ScalarUtils) {
	ASSERT_TYPE_EQ(Ctor<NonConstructible, double>, InputList);
	ASSERT_UNTRUE(fetch::pack::IsInvocableV<decltype(&f), fetch::pack::Nil>);
	ASSERT_UNTRUE(fetch::pack::IsInvocableV<decltype(&f), fetch::pack::TakeT<fetch::pack::SizeConstant<1>, InputList>>);
	ASSERT_UNTRUE(fetch::pack::IsInvocableV<decltype(&f), fetch::pack::TakeT<fetch::pack::SizeConstant<2>, InputList>>);
	ASSERT_UNTRUE(fetch::pack::IsInvocableV<decltype(&f), fetch::pack::TakeT<fetch::pack::SizeConstant<3>, InputList>>);
	ASSERT_HOLDS(fetch::pack::IsInvocableV<decltype(&f), InputList>);

	ASSERT_TYPE_EQ(fetch::pack::InvokeResultT<decltype(&f), InputList>, void);
	ASSERT_TYPE_EQ(fetch::pack::InvokeResultT<decltype(&g), fetch::pack::TakeT<fetch::pack::SizeConstant<3>, InputList>>, int);
}

TEST(PackTests, SwitchesAndSelects) {
	ASSERT_TYPE_EQ(fetch::pack::SwitchT<fetch::pack::Pack<
		       std::false_type, char,
		       std::false_type, int,
		       std::true_type, NonConstructible,
		       std::false_type, double>>,
		       NonConstructible);
	ASSERT_TYPE_EQ(fetch::pack::SwitchT<fetch::pack::Pack<
		       std::false_type, char,
		       std::false_type, int,
		       std::false_type, NonConstructible,
		       std::false_type, double,
		       InputList>>,
		       InputList);

	struct A {};
	struct B {};
	struct C { using type = int; };
	struct D {};

	ASSERT_TYPE_EQ(fetch::pack::SelectT<fetch::pack::Pack<A, B, C, D>>, int);
}

TEST(PackTests, Sort) {
	struct A: fetch::pack::SizeConstant<0> {};
	struct B: fetch::pack::SizeConstant<1> {};
	struct C: fetch::pack::SizeConstant<2> {};
	struct D: fetch::pack::SizeConstant<3> {};
	struct E: A {};

	//                                                        3  2  1  2  0  2  0  1                      0  1  2  3
	ASSERT_TYPE_EQ(fetch::pack::UniqueSortT<fetch::pack::Pack<D, C, B, C, A, C, E, B>>, fetch::pack::Pack<A, B, C, D>);
}

TEST(PackTests, Args) {
	ASSERT_TYPE_EQ(fetch::pack::ArgsT<std::is_constructible<char, int, NonConstructible, double>>, InputList);
	ASSERT_TYPE_EQ(fetch::pack::ArgsT<decltype(&f)>, InputList);
	ASSERT_TYPE_EQ(fetch::pack::ArgsT<decltype(&NonConstructible::M)>, InputList);
	ASSERT_TYPE_EQ(fetch::pack::ArgsT<decltype(&NonConstructible::CM)>, InputList);
}
