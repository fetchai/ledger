
#include<iostream>
#include"vectorize/register.hpp"
#include"vectorize/sse.hpp"

#include<random/lfg.hpp>

fetch::random::LinearCongruentialGenerator lcg;
using namespace fetch::vectorize;

void Test1() {
  alignas(16) uint32_t a[4] = {1,2,3,4};
  alignas(16) uint32_t b[4] = {2,4,8,16};
  alignas(16) uint32_t c[4] = {0};

  VectorRegister< uint32_t, 128 > r1(a), r2(b), r3;

  r3 = r1 * r2;
  r3 = r3 - r1;
  r3.Store(c);
  
  for(std::size_t i=0; i<4; ++i)
    std::cout << c[i] << " ";

  std::cout << std::endl;
}

void Test2() {
  alignas(16) float a[4] = {1,2,3,4};
  alignas(16) float b[4] = {2,4,8,16};
  alignas(16) float c[4] = {0};

  VectorRegister< float, 128 > r1(a), r2(b), r3, cst(3);

  r3 = r1 * r2;
  r3 = cst * r3 - r1;
  r3.Store(c);
  
  for(std::size_t i=0; i<4; ++i)
    std::cout << c[i] << " ";

  std::cout << std::endl;
}

    

int main() {
  alignas(16) double a[2] = {1,2};
  alignas(16) double b[2] = {2,4};
  alignas(16) double c[2] = {0};

  VectorRegister< double, 128> r1(a), r2(b), r3, cst(3.2);

  r3 = r1 * r2;
  r3 = cst*r3 - r1;
  r3.Store(c);
  
  for(std::size_t i=0; i<2; ++i)
    std::cout << c[i] << " ";

  std::cout << std::endl;
}
