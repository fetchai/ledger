#pragma once
#include <iostream>
#include <stack>

namespace fetch {
namespace math {
namespace linalg {

/*
 * This class uses the compiler to compile small 64-bit images
 * of expressions such as C = A * B + C into a rom. This number
 * can be used as a unique identifier for a given implementation
 * of various mathematical functions.
 *
 * This prototype framework is constant at compile time.
 */
template <uint64_t P, uint64_t S1>
struct Prototype
{
  static_assert(P <= 64, "stack overflow for const expression");
  enum
  {
    OpSize    = 4ull,
    StackSize = uint64_t(P),
    Stack     = uint64_t(S1)
  };

  enum
  {
    RET  = 0ull,
    MULT = 1ull,
    ADD  = 2ull,
    SUB  = 3ull,
    EQ   = 4ull,

    CONCAT    = 12ull,
    TRANSPOSE = 13ull,
    UPPER     = 14ull,
    LOWER     = 15ull
  };

  template <uint64_t OP>
  using one_op_return_type = Prototype<P + OpSize, uint64_t(S1) | (uint64_t(OP) << (P))>;

  template <typename O, uint64_t OP>
  using two_op_return_type =
      Prototype<P + O::StackSize + OpSize,
                uint64_t(S1) | (uint64_t(O::Stack) << P) | (uint64_t(OP) << (P + O::StackSize))>;

  template <typename O>
  two_op_return_type<O, ADD> constexpr operator+(O const &other) const
  {
    return two_op_return_type<O, ADD>();
  }

  template <typename O>
  two_op_return_type<O, MULT> constexpr operator*(O const &other) const
  {
    return two_op_return_type<O, MULT>();
  }

  template <typename O>
  two_op_return_type<O, RET> constexpr operator<=(O const &other) const
  {
    return two_op_return_type<O, RET>();
  }

  template <typename O>
  two_op_return_type<O, CONCAT> constexpr operator,(O const &other) const
  {
    return two_op_return_type<O, CONCAT>();
  }

  template <typename O>
  two_op_return_type<O, EQ> constexpr operator=(O const &other) const
  {
    return two_op_return_type<O, EQ>();
  }
};

Prototype<4, 0> const _A;      //< Represents Matrix 1
Prototype<4, 1> const _B;      //< Represents Matrix 2
Prototype<4, 2> const _C;      //< Represents Matrix 3
Prototype<4, 3> const _alpha;  //< Represents Scalar 1
Prototype<4, 4> const _beta;   //< Represents Scalar 2
Prototype<4, 5> const _gamma;  //< Represents Scalar 3
Prototype<4, 6> const _x;      //< Represents vector 1
Prototype<4, 7> const _y;      //< Represents vector 2
Prototype<4, 8> const _z;      //< Represents vector 3
Prototype<4, 9> const _m;      //< Represents integral 1
Prototype<4, 10> const _n;     //< Represents integral 2
Prototype<4, 11> const _p;     //< Represents integral 3


// Operatation representing the transposed of a matrix.
template <uint64_t P, uint64_t S>
constexpr typename Prototype<P, S>::template one_op_return_type<Prototype<P, S>::TRANSPOSE> T(
    Prototype<P, S> const &)
{
  return typename Prototype<P, S>::template one_op_return_type<Prototype<P, S>::TRANSPOSE>();
}

// Operatation defining the property "upper triangular" for a matrix
template <uint64_t P, uint64_t S>
constexpr typename Prototype<P, S>::template one_op_return_type<Prototype<P, S>::UPPER> U(
    Prototype<P, S> const &)
{
  return typename Prototype<P, S>::template one_op_return_type<Prototype<P, S>::UPPER>();
}

// Operatation defining the property "lower triangular" for a matrix
template <uint64_t P, uint64_t S>
constexpr typename Prototype<P, S>::template one_op_return_type<Prototype<P, S>::LOWER> L(
    Prototype<P, S> const &)
{
  return typename Prototype<P, S>::template one_op_return_type<Prototype<P, S>::LOWER>();
}

// Wrapper function to prettify the representation inside template constants
template <typename O>
constexpr uint64_t Computes(O const &)
{
  return O::Stack;
}

template <typename A>
constexpr uint64_t Computes(A const &a, A const &b)
{
  return Computes((a, b));
}

template <typename A, typename B, typename... O>
constexpr uint64_t Computes(A const &a, B const &b, O const &... objs)
{
  return Computes((a, b), objs...);
}


// Wrapper function to prettify signature representation.
template <typename O>
constexpr uint64_t Signature(O const &)
{
  return O::Stack;
}

template <typename A>
constexpr uint64_t Signature(A const &a, A const &b)
{
  return Signature((a, b));
}

template <typename A, typename B, typename... O>
constexpr uint64_t Signature(A const &a, B const &b, O const &... objs)
{
  return Signature((a, b), objs...);
}

}  // namespace linalg
}  // namespace math
}  // namespace fetch
