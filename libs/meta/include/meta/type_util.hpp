#pragma once
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

#include <type_traits>
#include <utility>

namespace fetch {
namespace type_util {

template <class... Ts>
struct Conjunction;

template <class... Ts>
static constexpr auto ConjunctionV = Conjunction<Ts...>::value;

template <class T, class... Ts>
struct Conjunction<T, Ts...>
{
  enum : bool
  {
    value = T::value && ConjunctionV<Ts...>
  };
};

template <>
struct Conjunction<>
{
  enum : bool
  {
    value = true
  };
};

template <template <class...> class F, class... Ts>
using All = Conjunction<F<Ts>...>;

template <template <class...> class F, class... Ts>
static constexpr auto AllV = All<F, Ts...>::value;

template <class... Ts>
struct Disjunction;

template <class... Ts>
static constexpr auto DisjunctionV = Disjunction<Ts...>::value;

template <class T, class... Ts>
struct Disjunction<T, Ts...>
{
  enum : bool
  {
    value = T::value || DisjunctionV<Ts...>
  };
};

template <>
struct Disjunction<>
{
  enum : bool
  {
    value = false
  };
};

template <template <class...> class F, class... Ts>
using Any = Disjunction<F<Ts>...>;

template <template <class...> class F, class... Ts>
static constexpr auto AnyV = Any<F, Ts...>::value;

template <template <class...> class F, class... Prefix>
struct Bind
{
  template <class... Args>
  using type = F<Prefix..., Args...>;
};

template <class T, class... Ts>
struct IsAnyOf
{
  enum : bool
  {
    value = AnyV<Bind<std::is_same, T>::template type, Ts...>
  };
};

template <class T, class... Ts>
static constexpr auto IsAnyOfV = IsAnyOf<T, Ts...>::value;

// Little pack utilities

template<class F, class... Args> struct InvokeResult
{
	using type = decltype(std::declval<F>()(std::declval<Args>()...));
};
template<class F, class... Args> using InvokeResultT = typename InvokeResult<F, Args...>::type;

namespace impl_ {

template<class F, class H, class... Ts> struct FoldL;
template<class F, class H, class... Ts> using FoldLResultT = typename FoldL<F, H, Ts...>::type;

template<class F, class Car, class Cadr, class... Cddr> struct FoldL<F, Car, Cadr, Cddr...>
{
	using type = FoldLResultT<F, InvokeResultT<F, Car, Cadr>, Cddr...>;
	static type call(F &&f, Car &&car, Cadr &&cadr, Cddr &&...cddr)
		noexcept(noexcept(FoldL<F, InvokeResultT<F, Car, Cadr>, Cddr...>::call(
			std::forward<F>(f), f(std::forward<Car>(car), std::forward<Cadr>(cadr)), std::forward<Cddr>(cddr)...)))
	{
		// no std::invoke yet
		return FoldL<F, InvokeResultT<F, Car, Cadr>, Cddr...>::call(
			std::forward<F>(f), f(std::forward<Car>(car), std::forward<Cadr>(cadr)), std::forward<Cddr>(cddr)...);
	}
};

template<class F, class First, class Last> struct FoldL<F, First, Last>
{
	using type = InvokeResultT<F, First, Last>;
	static type call(F &&f, First &&first, Last &&last)
		noexcept(noexcept(std::forward<F>(f)(std::forward<First>(first), std::forward<Last>(last))))
	{
		return std::forward<F>(f)(std::forward<First>(first), std::forward<Last>(last));
	}
};

template<class F, class Only> struct FoldL<F, Only>
{
	using type = std::remove_reference_t<Only>;
	static type call(F &&, Only &&only)
		noexcept(std::is_nothrow_move_constructible<Only>::value)
	{
		return std::forward<Only>(only);
	}
};

}

template<class F, class H, class... Ts>
inline constexpr auto FoldL(F &&f, H &&h, Ts &&...ts)
	noexcept(noexcept(impl_::FoldL<F, H, Ts...>::call(std::forward<F>(f), std::forward<H>(h), std::forward<Ts>(ts)...)))
{
  return impl_::FoldL<F, H, Ts...>::call(std::forward<F>(f), std::forward<H>(h), std::forward<Ts>(ts)...);
}

template<class RetVal> struct Const
{
  template<class> using type = RetVal;
};

}  // namespace type_util
}  // namespace fetch
