#pragma once
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

#include "meta/param_pack.hpp"

#include <cstddef>
#include <tuple>
#include <utility>

namespace fetch {
namespace meta {

template <std::size_t... Is>
using IndexSequence = std::index_sequence<Is...>;

namespace internal {

template <std::size_t, typename...>
struct SplitTuple;
template <std::size_t N, typename... Ts>
struct SplitTuple<N, std::tuple<Ts...>>
{
private:
  static_assert(N <= sizeof...(Ts), "Slice index exceeds tuple size");

  static constexpr std::size_t start_length = N;
  static constexpr std::size_t end_length   = sizeof...(Ts) - N;

  template <std::size_t... Is>
  static std::tuple<std::tuple_element_t<Is, std::tuple<Ts...>>...> get_start(IndexSequence<Is...>);
  template <std::size_t... Is>
  static std::tuple<std::tuple_element_t<N + Is, std::tuple<Ts...>>...> get_end(
      IndexSequence<Is...>);

public:
  using Initial  = decltype(get_start(std::make_index_sequence<start_length>()));
  using Terminal = decltype(get_end(std::make_index_sequence<end_length>()));

  static_assert(std::tuple_size<Initial>::value == start_length, "");
  static_assert(std::tuple_size<Terminal>::value == end_length, "");
};

template <typename>
struct TupleOperations;
template <typename... Ts>
struct TupleOperations<std::tuple<Ts...>>
{
  using type                        = std::tuple<Ts...>;
  static constexpr std::size_t size = std::tuple_size<type>::value;

  template <std::size_t N>
  using TakeInitial = TupleOperations<typename SplitTuple<N, type>::Initial>;
  template <std::size_t N>
  using DropInitial = TupleOperations<typename SplitTuple<N, type>::Terminal>;
  template <std::size_t N>
  using TakeTerminal =
      TupleOperations<typename SplitTuple<std::tuple_size<type>::value - N, type>::Terminal>;
  template <std::size_t N>
  using DropTerminal =
      TupleOperations<typename SplitTuple<std::tuple_size<type>::value - N, type>::Initial>;

  template <typename T>
  struct Prepend;
  template <typename... Ts2>
  struct Prepend<std::tuple<Ts2...>>
  {
    using type = std::tuple<Ts2..., Ts...>;
  };
  template <typename T>
  struct Append;
  template <typename... Ts2>
  struct Append<std::tuple<Ts2...>>
  {
    using type = std::tuple<Ts..., Ts2...>;
  };
};

template <typename>
struct Tuple;
template <>
struct Tuple<std::tuple<>> : TupleOperations<std::tuple<>>
{
};
template <typename T, typename... Ts>
struct Tuple<std::tuple<T, Ts...>> : TupleOperations<std::tuple<T, Ts...>>
{
  using FirstType = T;
  using LastType  = std::tuple_element_t<
      0, typename TupleOperations<std::tuple<T, Ts...>>::template TakeTerminal<1>::type>;
};

template <typename T>
struct IsStdTupleImpl : std::false_type
{
};
template <typename... Args>
struct IsStdTupleImpl<std::tuple<Args...>> : std::true_type
{
};

}  // namespace internal

template <typename T>
using Tuple = typename internal::Tuple<T>;

template <typename Tuple, template <typename...> class Destination>
using UnpackTuple = ConveyTypeParameterPack<std::tuple, Tuple, Destination>;

template <typename Tuple>
constexpr decltype(auto) IndexSequenceFromTuple()
{
  return std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>();
}

template <typename T>
constexpr bool IsStdTuple =
    internal::IsStdTupleImpl<std::remove_cv_t<std::remove_reference_t<T>>>::value;

}  // namespace meta
}  // namespace fetch
