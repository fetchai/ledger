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
    StackSize = uint64_t(P),
    Stack = uint64_t(S1)
  };
  
  enum {
    RET   =  0ull,
    MULT =  1ull,
    ADD  =  2ull,
    SUB  =  3ull,
    EQ   =  4ull,

    CONCAT = 12ull,    
    TRANSPOSE = 13ull,
    UPPER = 14ull,
    LOWER = 15ull
  };


  
  template< uint64_t OP >
  using one_op_return_type = Prototype<
    P + OpSize,
    uint64_t(S1) | (uint64_t(OP) << (P) )
      >;
  
  template< typename O, uint64_t OP >
  using two_op_return_type = Prototype<
    P + O::StackSize + OpSize,
    uint64_t(S1) | (uint64_t(O::Stack) << P) | (uint64_t(OP) << (P + O::StackSize) )
      >;
  
  template< typename O >
  two_op_return_type< O, ADD >
  constexpr operator+(O const &other) const {
    return two_op_return_type< O, ADD >();
  }

  template< typename O >
  two_op_return_type< O, MULT >
  constexpr operator*(O const &other) const {
    return two_op_return_type< O, MULT >();
  }

  template< typename O >
  two_op_return_type< O, RET >
  constexpr operator<=(O const &other) const {
    return two_op_return_type< O, RET >();
  }

  template< typename O >
  two_op_return_type< O, CONCAT >
  constexpr operator,(O const &other) const {
    return two_op_return_type< O, CONCAT >();
  }

  
  template< typename O >
  two_op_return_type< O, EQ >
  constexpr operator=(O const &other) const {
    return two_op_return_type< O, EQ >();
  }  
};

Prototype<4, 0> const _A;
Prototype<4, 1> const _B;
Prototype<4, 2> const _C;
Prototype<4, 3> const _alpha;
Prototype<4, 4> const _beta;
Prototype<4, 5> const _gamma;

template<uint64_t P, uint64_t S>
constexpr typename Prototype<P, S>::template one_op_return_type< Prototype<P, S>::TRANSPOSE > T(Prototype<P, S> const&) {
  return typename Prototype<P, S>::template one_op_return_type< Prototype<P, S>::TRANSPOSE >();
}

template<uint64_t P, uint64_t S>
constexpr typename Prototype<P, S>::template one_op_return_type< Prototype<P, S>::UPPER > U(Prototype<P, S> const&) {
  return typename Prototype<P, S>::template one_op_return_type< Prototype<P, S>::UPPER >();
}

template<uint64_t P, uint64_t S>
constexpr typename Prototype<P, S>::template one_op_return_type< Prototype<P, S>::LOWER > L(Prototype<P, S> const&) {
  return typename Prototype<P, S>::template one_op_return_type< Prototype<P, S>::LOWER >();
}



template<uint64_t P, uint64_t S>
std::ostream& operator<<(std::ostream& os, Prototype<P, S> const &prototype)  
{
  auto PrintSymbol = [&os](uint8_t const &op) {
    switch(op) {
    case 0:
      os << "_A ";
      break;
    case 1:
      os << "_B ";
      break;
    case 2:
      os << "_C ";
      break;
    case 3:
      os << "_alpha ";
      break;
    case 4:
      os << "_beta ";
      break;
    case 5:
      os << "_gamma ";
      break;
    case Prototype<P,S>::MULT:
      os << "* " ;
      break;
    case Prototype<P,S>::ADD:
      os << "+ ";
      break;
    case Prototype<P,S>::RET:
      os << "<= ";
      break;      
    case Prototype<P,S>::TRANSPOSE:
      os << "TRANS ";
      break;
    default:
      os << "?? " << int(op & 7) << " ";
    }
  };
  
  
  uint64_t s = S;
  std::stack< uint8_t > stack;

  for(std::size_t i = 0; i < P; i += 4) {
    uint8_t op = s & 15;
    s >>= 4;
    PrintSymbol(op);
  }

  return os;  
}


template< typename O >
constexpr uint64_t Computes(O const &) 
{
  return O::Stack;
}

template< typename O >
constexpr uint64_t Signature(O const&) 
{
  return O::Stack;
}


template< typename A >
constexpr uint64_t Signature(A const &a, A const &b) 
{
  return Signature( (a,b) );
}

template< typename A, typename B, typename... O >
constexpr uint64_t Signature(A const &a, B const &b, O const &...objs) 
{
  return Signature( (a,b), objs... );
}


}
}
}
