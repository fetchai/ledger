#ifndef RANDOM_HPP
#define RANDOM_HPP
#include"core/random/lcg.hpp"
#include"core/random/lfg.hpp"

namespace fetch
{
namespace random
{

struct Random 
{
  static LaggedFibonacciGenerator<> generator;
};

}
}

#endif
