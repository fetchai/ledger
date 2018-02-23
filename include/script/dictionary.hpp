#ifndef SCRIPT_DICTIONARY_HPP
#define SCRIPT_DICTIONARY_HPP
#include"memory/shared_hashtable.hpp"
#include"byte_array/referenced_byte_array.hpp"

#include<unordered_map>
#include<memory>

namespace fetch
{
namespace script
{


template< typename T >
class Dictionary
{
  struct Hash {
    std::size_t operator() (fetch::byte_array::BasicByteArray const  &key) const {
      uint32_t hash = 2166136261;
      for (std::size_t i = 0; i < key.size(); ++i)
      {
        hash = (hash * 16777619) ^ key[i];
      }
      
      return hash ;    
    }
  };

public:
  typedef byte_array::BasicByteArray key_type;
  typedef T type;
  typedef std::unordered_map< key_type, type, Hash > container_type;
  typedef std::shared_ptr< container_type > shared_container_type;  
  typedef typename container_type::iterator iterator;
  typedef typename container_type::const_iterator const_iterator;  
  

  
  Dictionary() {
    data_ = std::make_shared< container_type >( );
  }

  Dictionary(Dictionary const&dict) {
    data_ = dict.data_;    
  }
  
  Dictionary(Dictionary &&dict) {
    std::swap(data_, dict.data_);
  }  
  
  Dictionary<T> Copy() const
  {
    Dictionary<T> ret;

    for(auto it=data_->begin(); it!=data_->end(); ++it)
    {
      ret[it->first] = it->second;      
    }

    return ret;
  }
  
  type& operator[](byte_array::BasicByteArray const& key)
  {
    return (*data_)[key];
  }

  type const& operator[](byte_array::BasicByteArray const& key) const
  {
    return (*data_)[key];
  }

  iterator begin() 
  {
    return data_->begin();
  }
  
  iterator end() 
  {
    return data_->begin();
  }

  const_iterator begin() const
  {
    return data_->begin();
  }
  
  const_iterator end() const
  {
    return data_->begin();
  }
  
private:
  shared_container_type data_;
};


};
};

#endif
