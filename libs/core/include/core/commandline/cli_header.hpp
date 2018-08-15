#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include <iostream>
#include <string>
namespace fetch {
namespace commandline {
inline void DisplayCLIHeader(std::string const &name, std::string const &years = "2018",
                             std::string const &additional = "")
{
  std::cout << " F E ╱     " << name << std::endl;
  std::cout << "   T C     Copyright " << years << " (c) Fetch AI Ltd." << std::endl;
  std::cout << "     H     " << additional << std::endl << std::endl;
}
}  // namespace commandline
}  // namespace fetch
