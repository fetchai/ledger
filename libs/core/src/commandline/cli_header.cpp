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

#include "fetch_version.hpp"

#include <iostream>
#include <string>

namespace fetch {
namespace commandline {

void DisplayCLIHeader(std::string const &name, std::string const &years,
                      std::string const &additional)
{
  std::cout << " F E ╱     " << name << ' ' << version::FULL << '\n';
  std::cout << "   T C     Copyright " << years << " (c) Fetch AI Ltd." << '\n';
  std::cout << "     H     " << additional << '\n' << std::endl;
}

}  // namespace commandline
}  // namespace fetch
