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

#include "core/string/trim.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace fetch {
namespace string {

void TrimFromRight(std::string &s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return std::isspace(c) == 0; }).base(),
          s.end());
}

void TrimFromLeft(std::string &s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return std::isspace(c) == 0; }));
}

void Trim(std::string &s)
{
  TrimFromLeft(s);
  TrimFromRight(s);
}

}  // namespace string
}  // namespace fetch
