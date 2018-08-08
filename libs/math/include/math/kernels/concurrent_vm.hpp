#pragma once
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
