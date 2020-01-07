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

#include "meta/tuple.hpp"
#include "same.hpp"

#include "gmock/gmock.h"

#include <tuple>

namespace fetch {
namespace meta {

namespace {

using namespace ::testing;

struct A
{
};
struct B
{
};
struct C
{
};
struct D
{
};
struct E
{
};

class TupleMetaTests : public Test
{
};

using Input = std::tuple<A, B, C, D, E>;
using Empty = std::tuple<>;

TEST_F(TupleMetaTests, Tuple_noop)
{
  using input = Tuple<Input>::type;
  using empty = Tuple<Empty>::type;
  static_assert(Same<input, Input>, "");
  static_assert(Same<empty, Empty>, "");
}

TEST_F(TupleMetaTests, Tuple_TakeInitial_single_application)
{
  using empty = Tuple<Empty>::TakeInitial<0>::type;
  static_assert(Same<empty, Empty>, "");

  using initial_0 = Tuple<Input>::TakeInitial<0>::type;
  static_assert(Same<initial_0, Empty>, "");

  using initial_1 = Tuple<Input>::TakeInitial<1>::type;
  static_assert(Same<initial_1, std::tuple<A>>, "");

  using initial_3 = Tuple<Input>::TakeInitial<3>::type;
  static_assert(Same<initial_3, std::tuple<A, B, C>>, "");

  using initial_5 = Tuple<Input>::TakeInitial<5>::type;
  static_assert(Same<initial_5, Input>, "");
}

TEST_F(TupleMetaTests, Tuple_TakeTerminal_single_application)
{
  using empty = Tuple<Empty>::TakeTerminal<0>::type;
  static_assert(Same<empty, Empty>, "");

  using terminal_0 = Tuple<Input>::TakeTerminal<0>::type;
  static_assert(Same<terminal_0, Empty>, "");

  using terminal_1 = Tuple<Input>::TakeTerminal<1>::type;
  static_assert(Same<terminal_1, std::tuple<E>>, "");

  using terminal_3 = Tuple<Input>::TakeTerminal<3>::type;
  static_assert(Same<terminal_3, std::tuple<C, D, E>>, "");

  using terminal_5 = Tuple<Input>::TakeTerminal<5>::type;
  static_assert(Same<terminal_5, Input>, "");
}

TEST_F(TupleMetaTests, Tuple_DropInitial_single_application)
{
  using empty = Tuple<Empty>::DropInitial<0>::type;
  static_assert(Same<empty, Empty>, "");

  using initial_0 = Tuple<Input>::DropInitial<0>::type;
  static_assert(Same<initial_0, Input>, "");

  using initial_1 = Tuple<Input>::DropInitial<1>::type;
  static_assert(Same<initial_1, std::tuple<B, C, D, E>>, "");

  using initial_3 = Tuple<Input>::DropInitial<3>::type;
  static_assert(Same<initial_3, std::tuple<D, E>>, "");

  using initial_5 = Tuple<Input>::DropInitial<5>::type;
  static_assert(Same<initial_5, Empty>, "");
}

TEST_F(TupleMetaTests, Tuple_DropTerminal_single_application)
{
  using empty = Tuple<Empty>::DropTerminal<0>::type;
  static_assert(Same<empty, Empty>, "");

  using terminal_0 = Tuple<Input>::DropTerminal<0>::type;
  static_assert(Same<terminal_0, Input>, "");

  using terminal_1 = Tuple<Input>::DropTerminal<1>::type;
  static_assert(Same<terminal_1, std::tuple<A, B, C, D>>, "");

  using terminal_3 = Tuple<Input>::DropTerminal<3>::type;
  static_assert(Same<terminal_3, std::tuple<A, B>>, "");

  using terminal_5 = Tuple<Input>::DropTerminal<5>::type;
  static_assert(Same<terminal_5, Empty>, "");
}

TEST_F(TupleMetaTests, Tuple_TakeInitial_repeated_applications)
{
  using empty = Tuple<Empty>::TakeInitial<0>::TakeInitial<0>::type;
  static_assert(Same<empty, Empty>, "");

  using initial_00 = Tuple<Input>::TakeInitial<0>::TakeInitial<0>::type;
  static_assert(Same<initial_00, Empty>, "");

  using initial_11 = Tuple<Input>::TakeInitial<1>::TakeInitial<1>::type;
  static_assert(Same<initial_11, std::tuple<A>>, "");

  using initial_21  = Tuple<Input>::TakeInitial<2>::TakeInitial<1>::type;
  using initial_111 = Tuple<Input>::TakeInitial<1>::TakeInitial<1>::TakeInitial<1>::type;
  static_assert(Same<initial_21, std::tuple<A>>, "");
  static_assert(Same<initial_111, std::tuple<A>>, "");

  using initial_22 = Tuple<Input>::TakeInitial<2>::TakeInitial<2>::type;
  static_assert(Same<initial_22, std::tuple<A, B>>, "");
}

TEST_F(TupleMetaTests, Tuple_TakeTerminal_repeated_applications)
{
  using empty = Tuple<Empty>::TakeTerminal<0>::TakeTerminal<0>::type;
  static_assert(Same<empty, Empty>, "");

  using terminal_00 = Tuple<Input>::TakeTerminal<0>::TakeTerminal<0>::type;
  static_assert(Same<terminal_00, Empty>, "");

  using terminal_11 = Tuple<Input>::TakeTerminal<1>::TakeTerminal<1>::type;
  static_assert(Same<terminal_11, std::tuple<E>>, "");

  using terminal_21  = Tuple<Input>::TakeTerminal<2>::TakeTerminal<1>::type;
  using terminal_111 = Tuple<Input>::TakeTerminal<1>::TakeTerminal<1>::TakeTerminal<1>::type;
  static_assert(Same<terminal_21, std::tuple<E>>, "");
  static_assert(Same<terminal_111, std::tuple<E>>, "");

  using terminal_22 = Tuple<Input>::TakeTerminal<2>::TakeTerminal<2>::type;
  static_assert(Same<terminal_22, std::tuple<D, E>>, "");
}

TEST_F(TupleMetaTests, Tuple_DropInitial_repeated_applications)
{
  using empty = Tuple<Empty>::DropInitial<0>::DropInitial<0>::type;
  static_assert(Same<empty, Empty>, "");

  using initial_00 = Tuple<Input>::DropInitial<0>::DropInitial<0>::type;
  static_assert(Same<initial_00, Input>, "");

  using initial_11 = Tuple<Input>::DropInitial<1>::DropInitial<1>::type;
  static_assert(Same<initial_11, std::tuple<C, D, E>>, "");

  using initial_21  = Tuple<Input>::DropInitial<2>::DropInitial<1>::type;
  using initial_12  = Tuple<Input>::DropInitial<1>::DropInitial<2>::type;
  using initial_111 = Tuple<Input>::DropInitial<1>::DropInitial<1>::DropInitial<1>::type;
  static_assert(Same<initial_21, std::tuple<D, E>>, "");
  static_assert(Same<initial_12, std::tuple<D, E>>, "");
  static_assert(Same<initial_111, std::tuple<D, E>>, "");

  using initial_22 = Tuple<Input>::DropInitial<2>::DropInitial<2>::type;
  static_assert(Same<initial_22, std::tuple<E>>, "");
}

TEST_F(TupleMetaTests, Tuple_DropTerminal_repeated_applications)
{
  using empty = Tuple<Empty>::DropTerminal<0>::DropTerminal<0>::type;
  static_assert(Same<empty, Empty>, "");

  using terminal_00 = Tuple<Input>::DropTerminal<0>::DropTerminal<0>::type;
  static_assert(Same<terminal_00, Input>, "");

  using terminal_11 = Tuple<Input>::DropTerminal<1>::DropTerminal<1>::type;
  static_assert(Same<terminal_11, std::tuple<A, B, C>>, "");

  using terminal_21  = Tuple<Input>::DropTerminal<2>::DropTerminal<1>::type;
  using terminal_12  = Tuple<Input>::DropTerminal<1>::DropTerminal<2>::type;
  using terminal_111 = Tuple<Input>::DropTerminal<1>::DropTerminal<1>::DropTerminal<1>::type;
  static_assert(Same<terminal_21, std::tuple<A, B>>, "");
  static_assert(Same<terminal_12, std::tuple<A, B>>, "");
  static_assert(Same<terminal_111, std::tuple<A, B>>, "");

  using terminal_22 = Tuple<Input>::DropTerminal<2>::DropTerminal<2>::type;
  static_assert(Same<terminal_22, std::tuple<A>>, "");
}

TEST_F(TupleMetaTests, Tuple_Take_Drop_mixed)
{
  using empty = Tuple<Empty>::TakeTerminal<0>::DropTerminal<0>::type;
  static_assert(Same<empty, Empty>, "");

  using TupleA =
      Tuple<Input>::DropTerminal<1>::TakeTerminal<3>::TakeInitial<3>::DropInitial<1>::type;
  static_assert(Same<TupleA, std::tuple<C, D>>, "");
}

TEST_F(TupleMetaTests, Tuple_Append_Prepend)
{
  using empty1 = Tuple<Empty>::Prepend<Empty>::type;
  using empty2 = Tuple<Empty>::Append<Empty>::type;
  static_assert(Same<empty1, Empty>, "");
  static_assert(Same<empty2, Empty>, "");

  using TupleA = Tuple<std::tuple<A, B>>::Prepend<std::tuple<C, D>>::type;
  static_assert(Same<TupleA, std::tuple<C, D, A, B>>, "");

  using TupleB = Tuple<std::tuple<A, B>>::Append<std::tuple<C, D>>::type;
  static_assert(Same<TupleB, std::tuple<A, B, C, D>>, "");
}

}  // namespace

}  // namespace meta
}  // namespace fetch
