#ifndef MINER_HPP
#define MINER_HPP

#include <random>

namespace fetch {
namespace chain {
namespace consensus {

class DummyMiner
{

public:
  // Blocking mine
  template <typename block_type>
  static void Mine(block_type &block)
  {
    uint64_t initNonce = GetRandom();
    block.body().nonce = initNonce;

    block.UpdateDigest();

    while(!block.proof()())
    {
      block.body().nonce++;
      block.UpdateDigest();
    }
  }

  // Mine for set number of iterations
  template <typename block_type>
  static bool Mine(block_type &block, uint64_t iterations)
  {
    uint32_t initNonce = GetRandom();
    block.body().nonce = initNonce;

    block.UpdateDigest();

    while(!block.proof()() && iterations > 0)
    {
      block.body().nonce++;
      block.UpdateDigest();
      iterations--;
    }

    return block.proof()();
  }

private:
  static uint32_t GetRandom()
  {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(
        0, std::numeric_limits<uint32_t>::max());
    return dis(gen);
  }

};
}
}
}
#endif
