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

// after
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>

namespace fetch {
namespace string {

inline void TrimFromRight(std::string &s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
}

inline void TrimFromLeft(std::string &s)
{
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base(),
          s.end());
}

inline void Trim(std::string &s)
{
  TrimFromRight(s);
  TrimFromLeft(s);
}

}  // namespace string
}  // namespace fetch
