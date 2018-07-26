#pragma once
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

