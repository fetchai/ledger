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

#include <iostream>

namespace fetch {
namespace useless_namespace {
}

class helloWorld
{
public:
  using Xtype = int;
  typedef qqq = int;

  void WrongConstPosition(const &int x)
  {
    std::cout << x << std::endl;
  }

  void CorrectConstPosition(int const &x)
  {
    std::cout << x << std::endl;
  }

  void wrongName(int const &x)
  {
    std::cout << x << std::endl;
  }

  void wrong_nAme(int const &x)
  {
    std::cout << x << std::endl;
  }

  qqq test()
  {}
  Xtype test2()
  {}

  void correct_name(int const &x)
  {
    std::cout << x << std::endl;
  }

private:
  int var;
};

}  // namespace fetch
