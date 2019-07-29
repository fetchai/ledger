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

#include "meta/type_util.hpp"
#include "meta/value_util.hpp"

#include <type_traits>
#include <utility>

namespace fetch {
namespace type_util {

// In this header, a Switch struct is implemented. In brief, it should be used as:
//
// 1. First, one needs to define `case labels' (Ids in the code below).
//    A typical `case label' is a struct that has at least two members:
//    a) static const(expr)ant value, similar to the real switch statement labels;
//    b) static method Invoke that, when invoked as Invoke(f, args...),
//       is expected to (perhaps) invoke f on appropriate views of args.
//    See Setter struct in type_util_tests.cpp, prior to TypeUtilTests.Switch,
//    as a simple example.
//
// 2. In addition, you may define a default handler, analogous to `default:' label
//    of switch statements. It only needs to provide Invoke method, similar to that of case labels.
//
// 3. Next, the Switch struct itself is instantiated as
//
//       using MySwitch = Switch<Case1, Case2...[, DefaultCase<Default>]>;
//
//    where CaseN is a `case label', as described in 1., and Default is the default handler from 2.
//    The order of cases is insignificant (equally, the default case can be at any position among
//    the parameters), but if two different cases share the same ::value, the rightmost one of them
//    is ignored. The default case may be omitted; then DefaultCase<ReturnZero> is used by default:
//    ReturnZero::Invoke(f, args...) simply returns the value-initialized return type of f when
//    invoked on args (a no-op in case of void return type).
//    Any subsequence of Cases (incl. the Default one) can be packed in fetch::pack::Pack.
//
// 4. Finally, the switch statement is performed as
//
//       MySwitch::Invoke(selector, f, args...);
//
//    where
//
//       - selector is a numeric value, less than- and equal-comparable against each of the Cases;
//       - f is a templated function that can be invoked on arg views instantiated by each of the
//           Cases (a lambda with auto args, for instance);
//       - args... are the viewed args themselves.
//
// EXAMPLE
//
//    using namespace fetch::vm;
//    using namespace fetch::type_util;
//
//    Variant twenty(std::int16_t(20), TypeIds::Int16);
//    Variant twenty_two(std::int16_t(22), TypeIds::Int16);
//
//    template<TypeId type_id> struct VariantTypeId
//        // parent defines the ::value
//      : std::integral_constant<TypeId, type_id>
//    {
//      // not defined here for brevity (some additional machinery is needed for vm Variants)
//      template<class F> static auto Invoke(F &&f, Variant &a, Variant &b);
//    };
//
//    using SignedIntSeq = std::integer_sequence<TypeId, TypeIds::Int8, TypeIds::Int16,
//                                                       TypeIds::Int32, TypeIds::Int64>;
//    using SignedIntIds = LiftIntegerSequence<SignedIntSeq>;
//                         // defined at the bottom of this header
//
//    auto sum = Switch<SignedIntIds>::Invoke(twenty.type_id,
//        [](auto &&a, auto &&b)
//        {
//          return a + b;
//        },
//        twenty, twenty_two);
//
// Kindly disregard the heavily overloaded semantics of the word `default' in the whole text above.

// Default case for Switches.
// It is but a tagging wrapper for a particular implementation.
// The implementation itself is expected to provide the required Invoke member function
// (see ReturnZero below for an example).
template <class Implementation>
struct DefaultCase : Implementation
{
  using Implementation::Implementation;
};

}  // namespace type_util

namespace pack {

// Make DefaultCase -inf numerically so that it is always the first member of a sorted ids pack.
template <class Impl, class T>
struct LessThan<type_util::DefaultCase<Impl>, T> : std::true_type
{
};
template <class T, class Impl>
struct LessThan<T, type_util::DefaultCase<Impl>> : std::false_type
{
};
template <class Impl1, class Impl2>
struct LessThan<type_util::DefaultCase<Impl1>, type_util::DefaultCase<Impl2>> : std::true_type
{
};

}  // namespace pack

namespace type_util {

namespace detail_ {

// When no default case provided, Switch defaults to ReturnZero.
// ReturnZero::Invoke(f, args...) returns value-constructed invoke result of f over args.
// FirstCase should be the real case alternative of a Switch and is used to infer the return type.
template <class FirstCase>
class ReturnZero
{
  template <class F, class... Args>
  using InvokeResultT = decltype(FirstCase::Invoke(std::declval<F>(), std::declval<Args>()...));

  template <class T>
  using IsNothrowConstructible =
      type_util::Or<std::is_same<T, void>, std::is_nothrow_constructible<T>>;

public:
  template <class F, class... Args>
  static constexpr auto Invoke(F &&, Args &&...) noexcept(
      IsNothrowConstructible<InvokeResultT<F, Args...>>::value)
  {
    return InvokeResultT<F, Args...>();
  }
};

// Large (more than four) id packs are stored in binary search trees,
// to reduce run-time case alternative search time to logarithmic.
// SwitchNode is a root of a (sub-)tree, Left and Right being Switches themselves.
template <class Left, class Right>
struct SwitchNode
{
  /**
   * Manifests the lowest id this subtree can handle (the least case label).
   * This simply passes control to Left::LowerBound(); consecutive calls finally
   * lead to the lowest id of the whole Switch parameter pack.
   *
   * @return
   */
  static constexpr auto LowerBound() noexcept
  {
    return Left::LowerBound();
  }

  /**
   * Passes f and args... to the appropriate branch.
   *
   * @param selector The exact id to be matched against known cases, similar to `<expr>' in
   * `switch(<expr>)'
   * @param f The templated callable to be invoked on arg views
   * @param args
   * @return Whatever f returns on selector-resolved views of args
   */
  template <class Id, class F, class... Args>
  static constexpr decltype(auto) Invoke(Id selector, F &&f, Args &&... args)
  {
    //
    if (selector >= Right::LowerBound())
    {
      return Right::Invoke(selector, std::forward<F>(f), std::forward<Args>(args)...);
    }
    return Left::Invoke(selector, std::forward<F>(f), std::forward<Args>(args)...);
  }
};

// Internal switch implementation prepends ReturnZero as a DefaultCase to cases pack.
template <class Ids>
struct Switch : Switch<pack::ConcatT<DefaultCase<ReturnZero<pack::HeadT<Ids>>>, Ids>>
{
};

// When pack is large (> 4 ids), it is split in two (almost) equal branches.
// (The left branch can be one id less than the right one, with the odd amount of cases.)
template <class Default, class Ids>
using BinarySwitch = SwitchNode<Switch<pack::ConsT<Default, pack::LeftHalfT<Ids>>>,
                                Switch<pack::ConsT<Default, pack::RightHalfT<Ids>>>>;

// LinearSwitch performs a simple linear lookup. It is used with small amount of cases.
// For instance Switch<Case1, ..., Case6> would be implemented as
// BinarySwitch<LinearSwitch<Case1, ..., Case3>, LinearSwitch<Case4, ..., Case6>>.
template <class Default, class Ids>
class LinearSwitch
{
  template <class RemainingIds, class Id, class F, class... Args>
  static constexpr decltype(auto) Match(RemainingIds, Id selector, F &&f, Args &&... args);

  /**
   * When Ids pack is exhausted (the selector id did not match anyone)
   * control is passed to this method that invokes the Default case.
   *
   * @param _ anonymous param of type pack::Nil (Ids pack is exausted)
   * @param _ anonymous param of type Id (Default ignores any ids)
   * @param f The templated callable to be invoked on default arg views
   * @param args
   * @return Whatever Default::Invoke returns
   */
  template <class Id, class F, class... Args>
  static constexpr decltype(auto) Match(pack::Nil, Id, F &&f, Args &&... args)
  {
    return Default::Invoke(std::forward<F>(f), std::forward<Args>(args)...);
  }
  using Leftmost = HeadT<Ids>;

public:
  /**
   * Manifests the lowest id this subtree can handle (the least case label).
   * This simply passes control to Left::LowerBound(); consecutive calls finally
   * lead to the lowest id of the whole Switch parameter pack.
   *
   * @return
   */
  static constexpr auto LowerBound() noexcept
  {
    return Leftmost::value;
  }

  /**
   * Passes f and args... to the appropriate case alternative, or default if none matches.
   *
   * @param selector The exact id to be matched against known cases, similar to `<expr>' in
   * `switch(<expr>)'
   * @param f The templated callable to be invoked on arg views
   * @param args
   * @return Whatever f returns on selector-resolved views of args
   */
  template <class Id, class F, class... Args>
  static constexpr decltype(auto) Invoke(Id selector, F &&f, Args &&... args)
  {
    return Match(Ids{}, selector, std::forward<F>(f), std::forward<Args>(args)...);
  }
};

/**
 * Attempts to match selector against the head of current Ids list.
 * If id's value is different, invokes instantiation with Ids' tail.
 *
 * @param _ anonymous param of type Ids (ids pack to be matched against selector)
 * @param selector The exact id to be matched against Ids' head
 * @param f The templated callable to be invoked on arg views
 * @param args
 * @return Whatever f returns on views of args, if selector matches;
 * the template-recursive call otherwise
 */
template <class Default, class Ids>
template <class RemainingIds, class Id, class F, class... Args>
constexpr decltype(auto) LinearSwitch<Default, Ids>::Match(RemainingIds, Id selector, F &&f,
                                                           Args &&... args)
{
  using CurrentId = pack::HeadT<RemainingIds>;
  if (selector == CurrentId::value)
  {
    return CurrentId::Invoke(std::forward<F>(f), std::forward<Args>(args)...);
  }
  return Match(pack::TailT<RemainingIds>{}, selector, std::forward<F>(f),
               std::forward<Args>(args)...);
}

// The full internal Switch implementation, with default case.
template <class DefaultImpl, class... Ids>
struct Switch<pack::Pack<DefaultCase<DefaultImpl>, Ids...>>
  : std::conditional_t<(sizeof...(Ids) < 5), LinearSwitch<DefaultImpl, pack::Pack<Ids...>>,
                       BinarySwitch<DefaultImpl, pack::Pack<Ids...>>>
{
};

}  // namespace detail_

// Ids... are packed in a pack::Pack and sorted in ascending order, duplicates removed.
// (Note that DefaultCase, if specified, would end up at the head of the list.)
template <class... Ids>
using Switch = detail_::Switch<pack::UniqueSortT<pack::ConcatT<Ids...>>>;

// This one may prove useful, so that we could convert a single integral_sequence
// (or a similar template instantiation) into a pack of singleton integer_sequences,
// to be used as case alternatives for a Switch.
template <class Sequence>
struct LiftIntegerSequence;
template <class Sequence>
using LiftIntegerSequenceT = typename LiftIntegerSequence<Sequence>::type;
template <template <class Id, Id...> class Ctor, class Id, Id... ids>
struct LiftIntegerSequence<Ctor<Id, ids...>>
  : TypeConstant<pack::Pack<std::integer_sequence<Id, ids>...>>
{
};

}  // namespace type_util
}  // namespace fetch
