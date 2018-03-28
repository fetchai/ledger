#ifndef CRYPTO_MERKLE_SET_HPP
#define CRYPTO_MERKLE_SET_HPP

#include"byte_array/referenced_byte_array.hpp"

#include<stack>

namespace fetch
{
namespace crypto
{

struct MerkleNode
{
  enum {
    UNDEFINED = uint64_t(-1)
  };  

  ByteArray hash;    
  uint64_t left = UNDEFINED, right = UNDEFINED;
};
  


class MerkleSet
{
public:
  typedef byte_array::ByteArray byte_array_type;
  
  MerkleSet() 
  {
    
  }
  
  
  void Insert(ByteArray const &key) 
  {
    MerkleNode n;
    n.hash = key;
    std::size_t i = 0;
    tree_.push_back(n);

    std::stack< uint32_t >
  }
  

  byte_array_type hash() const {
    
  }
  
private:
  uint64_t root_ = MerkleNode::UNDEFINED;  
  std::vector< MerkleNode > tree_;
};

  



};
};


#endif
