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
#include "same.hpp"

#include "gmock/gmock.h"

#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>

using namespace ::testing;

namespace fetch {
namespace meta {

namespace {

using namespace ::testing;

template <typename... Args>
struct From
{
};
template <typename... Args>
struct To
{
  constexpr static std::size_t count = sizeof...(Args);
};

class ParamPackTests : public Test
{
};

TEST_F(ParamPackTests, ConveyTypeParameterPack_count)
{
  using Empty = ConveyTypeParameterPack<From, From<>, To>;
  static_assert(Empty::count == 0, "");

  using NonEmpty = ConveyTypeParameterPack<From, From<int, std::string>, To>;
  static_assert(NonEmpty::count == 2, "");
}

TEST_F(ParamPackTests, ConveyTypeParameterPack_test_unqualified_types)
{
  using SourceType        = From<int, std::string>;
  using ExpectedFinalType = std::tuple<int, std::string>;

  using IntermediateType = ConveyTypeParameterPack<From, SourceType, To>;
  using ActualFinalType  = ConveyTypeParameterPack<To, IntermediateType, std::tuple>;

  static_assert(Same<ActualFinalType, ExpectedFinalType>, "");
}

TEST_F(ParamPackTests, ConveyTypeParameterPack_test_qualified_types)
{
  using SourceType        = From<int &&, char *, std::string const &>;
  using ExpectedFinalType = std::tuple<int &&, char *, std::string const &>;

  using IntermediateType = ConveyTypeParameterPack<From, SourceType, To>;
  using ActualFinalType  = ConveyTypeParameterPack<To, IntermediateType, std::tuple>;

  static_assert(Same<ActualFinalType, ExpectedFinalType>, "");
}

}  // namespace

}  // namespace meta
}  // namespace fetch
