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

#include "core/string/split.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace core {

std::vector<std::string> Split(std::string const &input, std::string const &separator)
{
  if (separator.empty())
  {
    return {input};
  }

  std::vector<std::string> output;
  std::size_t              current  = 0;
  std::size_t              previous = 0;

  current = input.find(separator, previous);
  while (current != std::string::npos)
  {
    auto segment = input.substr(previous, current - previous);
    output.push_back(std::move(segment));
    previous = current + separator.size();
    current  = input.find(separator, previous);
  }

  auto const segment = input.substr(previous, current - previous);
  output.push_back(segment);

  return output;
}

}  // namespace core
}  // namespace fetch
