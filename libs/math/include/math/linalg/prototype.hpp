#pragma once
#include<iostream>
#include<stack>

namespace fetch
{
namespace math 
{
namespace linalg 
{

template<uint64_t P, uint64_t S1 >
struct Prototype
{
  static_assert( P <= 64, "stack overflow for const expression");
  enum {
    OpSize = 4ull,
    StackSize = P,
    Stack1 = S1
  };
  
  enum {
    IS_OP = 1ull << (OpSize - 1 ),
    MULT = 1ull | IS_OP,
    ADD =  2ull | IS_OP,
  
    TRANSPOSE = 4ull | IS_OP
  };

  
  template< typename O, uint64_t OP >
  using two_op_return_type = Prototype<
    			P + O::StackSize + OpSize,
    			S1 | (O::Stack1 << P) | (OP << (P + O::StackSize) )
      >;

  template< uint64_t OP >
  using one_op_return_type = Prototype<
    			P + OpSize, 
    			S1 | (OP << P)
      		>;

  template< typename O >
  two_op_return_type< O, MULT >
  constexpr operator*(O const &other) const {
    return two_op_return_type< O, MULT >();
  }

  template< typename O >
  two_op_return_type< O, ADD >
  constexpr operator*(O const &other) const {
    return two_op_return_type< O, ADD >();
  }

    
};

Prototype<4, 0> const _A;
Prototype<4, 1> const _B;
Prototype<4, 2> const _C;
Prototype<4, 3> const _alpha;
Prototype<4, 4> const _beta;
Prototype<4, 5> const _gamma;

template<uint64_t P, uint64_t S>
std::ostream& operator<<(std::ostream& os, Prototype<P, S> const &prototype)  
{

  uint64_t s = S;
  std::stack< uint8_t > stack;
  for(std::size_t i = 0; i < P; i += 4) {
    uint8_t op = s & 15;
    s >>= 4;

    switch(op) {
    case 0:
      std::cout << "_A ";
      break;
    case 1:
      std::cout << "_B ";
      break;
    case 2:
      std::cout << "_C ";
      break;
    case 3:
      std::cout << "_alpha ";
      break;
    case 4:
      std::cout << "_beta ";
      break;
    case 5:
      std::cout << "_gamma ";
      break;
    case Prototype<P,S>::MULT:
      std::cout << "* ";
      break;
    case Prototype<P,S>::ADD:
      std::cout << "+ ";
      break;
    case Prototype<P,S>::TRANSPOSE:
      std::cout << "TRANS ";
      break;
      
    }
  }
  
  return os;  
}  


template< typename O >
constexpr uint64_t Computes(O const &) 
{
  return O::Stack1;
}


}
}
}