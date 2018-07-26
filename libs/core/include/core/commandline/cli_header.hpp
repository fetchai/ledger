#pragma once
#include <iostream>
#include <string>
namespace fetch {
namespace commandline {
inline void DisplayCLIHeader(std::string const &name,
                             std::string const &years      = "2018",
                             std::string const &additional = "")
{
  std::cout << " F E ╱     " << name << std::endl;
  std::cout << "   T C     Copyright " << years << " (c) Fetch AI Ltd."
            << std::endl;
  std::cout << "     H     " << additional << std::endl << std::endl;
}
}  // namespace commandline
}  // namespace fetch

