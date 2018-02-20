#ifndef SCRIPT_DICTIONARY_HPP
#define SCRIPT_DICTIONARY_HPP
#include"memory/shared_hashtable.hpp"

namespace fetch
{
namespace script
{


template< typename T >
class Dictionary 
{
public:
  typedef T type;

  
private:
  uint32_t GetBucket(byte_array::BasicByteArray const& key) 
  {
    uint32_t h = 0;
    for(std::size_t i = 0; i < key.size(); ++i)
    {
        h ^= (h << 5) + (h >> 2) + key[i];
    }

    return h & (( 1<<  log_buckets_) -1);
  }

  std::size_t log_buckets_ = 0;  
  std::vector< SharedArray< details::Record< T >  > buckets_;
  
}
  



};
};

#endif
