#ifndef BLOCK_HEADERS_NODE_INTERFACE__
#define BLOCK_HEADERS_NODE_INTERFACE__

#include <iostream>
#include <string>

#include "ledger/chain/main_chain.hpp"

namespace fetch
{
namespace chain
{

class BlockHeadersNodeInterface
{
public:
  typedef fetch::byte_array::ByteArray block_identifier_type;
  typedef fetch::chain::MainChain::block_type block_header_data_type;

  BlockHeadersNodeInterface()
  {
  }

  virtual ~BlockHeadersNodeInterface()
  {
  }

  std::list<std::pair<block_identifier_type, block_header_data_type>> GetRecentBlockHeaders(unsigned int maxcount) = 0;
  std::list<block_header_data_type> GetSpecificBlockHeader(const block_identifier_type &hash) = 0;
};

}
}

#endif

