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

#include <cstddef>
#include <tuple>
#include <utility>

namespace fetch {
namespace meta {

namespace internal {

template <template <typename...> class, typename, template <typename...> class>
struct ConveyTypeParameterPackImpl;
template <template <typename...> class Source, template <typename...> class Destination,
          typename... Args>
struct ConveyTypeParameterPackImpl<Source, Source<Args...>, Destination>
{
  using type = Destination<Args...>;
};

}  // namespace internal

/**
 * Accepts a class template of the form Source<typename ...Args>, a type which
 * is an instantiation Source<Args...>, and another class template
 * Target<typename ...Args>.
 *
 * Evaluates to a type which is the instantiation Target<Args...>
 *
 * Example:
 *   template <typename ... Ts>
 *   struct NeedsParamPackToWork {...}
 *
 *   template <typename ArbitraryStdTuple>
 *   void doStuffWithTuples(ArbitraryStdTuple&&)
 *   {
 *     // uses the same param pack that was used to make the tuple
 *     using Instantiation = ConveyTypeParameterPack<
 *       std::tuple,
 *       ArbitraryStdTuple,
 *       NeedsParamPackToWork>;
 *   }
 */
template <template <typename...> class Source, typename SourceInstantiation,
          template <typename...> class Destination>
using ConveyTypeParameterPack =
    typename internal::ConveyTypeParameterPackImpl<Source, SourceInstantiation, Destination>::type;

}  // namespace meta
}  // namespace fetch
