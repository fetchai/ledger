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

#include <memory>
#include <string>

namespace fetch {
namespace semanticsearch {

struct VocabularyLocation
{
  std::string      model;
  SemanticPosition position;

  bool operator<(VocabularyLocation const &other) const
  {
    if (model == other.model)
    {
      std::size_t i = 0, n = std::min(position.size(), other.position.size());
      while ((i < n) && (position[i] == other.position[i]))
      {
        ++i;
      }

      if (i >= n)
      {
        return false;
      }

      return position[i] < other.position[i];
    }

    return model < other.model;
  }
};

}  // namespace semanticsearch
}  // namespace fetch
