#ifndef MINER_HPP
#define MINER_HPP

namespace fetch {
namespace chain {
namespace consensus {

class DummyMiner
{

public:
  template <typename block_type>
  static void Mine(block_type &block)
  {
    while(!block.proof()())
    {
      block.body().nonce++;
      block.UpdateDigest();
    }
  }

  template <typename block_type>
  static bool Mine(block_type &block, uint64_t iterations)
  {
    while(!block.proof()() && iterations > 0)
    {
      block.body().nonce++;
      block.UpdateDigest();
      iterations--;
    }

    return block.proof()();
  }

};
}
}
}
#endif
