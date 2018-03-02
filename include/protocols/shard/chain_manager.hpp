#ifndef PROTOCOLS_SHARD_CHAIN_MANAGER_HPP
#define PROTOCOLS_SHARD_CHAIN_MANAGER_HPP

#include"crypto/fnv.hpp"
#include"chain/transaction.hpp"
#include"assert.hpp"

#include<unordered_set>

namespace fetch
{
namespace protocols
{

class ChainManager 
{
public:
  typedef crypto::CallableFNV hasher_type;


  void SwitchBranch(block_type new_head, block_type old_head)
  {
    
  }
  
  
  block_type const &head() const 
  {
    return head_;    
  }
  
private:
  
};


};
};


#endif
