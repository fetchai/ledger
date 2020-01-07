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

#include "semanticsearch/index/base_types.hpp"

namespace fetch {
namespace semanticsearch {

struct ScopeIdentifier
{
  std::string            address{""};
  SemanticCoordinateType version{-1};

  bool operator<(ScopeIdentifier const &other) const
  {
    if (address == other.address)
    {
      return version < other.version;
    }

    return address < other.address;
  }

  bool operator==(ScopeIdentifier const &other) const
  {
    return (address == other.address) && (version == other.version);
  }
};

}  // namespace semanticsearch
}  // namespace fetch
