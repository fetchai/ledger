
#include<iostream>
#include"vectorize/register.hpp"
#include"vectorize/sse.hpp"

#include<random/lfg.hpp>

fetch::random::LinearCongruentialGenerator lcg;
using namespace fetch::vectorize;

int main() {
  alignas(16) uint32_t a[4] = {1,1,1,1};
  alignas(16) uint32_t b[4] = {2,4,8,16};
  alignas(16) uint32_t c[4] = {0};

  typedef typename VectorMemory< uint32_t, InstructionSet::X86_SSE3 >::register_type reg;
  VectorMemory< uint32_t, InstructionSet::X86_SSE3 > r1(a), r2(b), r3(c);
  std::cout << reg::E_REGISTER_SIZE << " " << reg::E_BLOCK_COUNT << std::endl;
  
  r3[0] = r1[0] * r2[0];
  for(std::size_t i=0; i<4; ++i)
    std::cout << c[i] << " ";

  std::cout << std::endl;
}
