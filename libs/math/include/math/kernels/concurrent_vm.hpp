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

#include <vector>

namespace fetch {
namespace kernels {

template <typename T>
struct ConcurrentVM
{
  std::vector<uint32_t> instructions;
  void                  operator()(T const &reg1, T const &reg2, T &reg3) const
  {
    int pc = 0;
    T   regs[3];
    regs[0] = reg1;
    regs[1] = reg2;
    regs[2] = reg3;

    while (pc < instructions.size())
    {
      uint32_t inst = instructions[pc];
      ++pc;

      T &r1 = regs[(inst >> 8) & 255];
      T &r2 = regs[(inst >> 16) & 255];
      T &r3 = regs[(inst >> 24) & 255];
      switch (inst & 0xFF)
      {
      case 0:
        break;
      case 1:
        r3 = r1 + r2;
        break;
      case 2:
        r3 = r1 - r2;
        break;
      case 3:
        r3 = r1 * r2;
        break;
      case 4:
        r3 = r1 / r2;
        break;
      }
    }

    reg3 = regs[2];
  }

  void AddInstruction(uint32_t const &inst, uint32_t const &reg1, uint32_t const &reg2,
                      uint32_t const &reg3)
  {
    uint32_t i = (inst & 255) | ((reg1 & 255) << 8) | ((reg2 & 255) << 16) | ((reg3 & 255) << 24);

    instructions.push_back(i);
  }
};

}  // namespace kernels
}  // namespace fetch
