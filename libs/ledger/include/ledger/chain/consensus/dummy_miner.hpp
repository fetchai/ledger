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

};
}
}
}
#endif
