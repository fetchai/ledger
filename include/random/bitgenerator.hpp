#ifndef RANDOM_BITGENERATOR_HPP
#define RANDOM_BITGENERATOR_HPP
#include"random/bitmask.hpp"
#include"random/lfg.hpp"
namespace fetch
{
namespace random
{

template< typename R = LaggedFibonacciGenerator<>, uint8_t B = 12, bool MSBF = true  >
class BitGenerator {
public:
  typedef R random_generator_type;
  typedef typename random_generator_type::random_type word_type;
  typedef BitMask< word_type, B, MSBF > mask_type;

  word_type operator()(mask_type const &m) {
    word_type s = word_type(-1);
    word_type r = m[0];
    
    for(std::size_t i=1 ; s && (i < mask_type::BITS_OF_PRECISION) ; ++i) {
      s &= rng_();
      r ^= (m[i] & s);
    }
    return r;
  }

  word_type Seed(word_type s) { return rng_.Seed(s); }
private:
  random_generator_type rng_;
};
  
  
}
}
#endif
