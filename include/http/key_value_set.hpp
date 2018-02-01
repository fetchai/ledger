#ifndef HTTP_KEYVALUE_SET
#define HTTP_KEYVALUE_SET
#include"byte_array/const_byte_array.hpp"

#include<vector>
namespace fetch {
namespace http {

struct KeyValuePair
{
  byte_array::ConstByteArray key;
  byte_array::ConstByteArray value;
};

  
class KeyValueSet : private std::vector< KeyValuePair >
{
public:
  typedef byte_array::ConstByteArray byte_array_type;
  
  typedef typename std::vector< KeyValuePair >::iterator iterator;
  typedef typename std::vector< KeyValuePair >::const_iterator const_iterator;

  iterator begin() noexcept { return std::vector< KeyValuePair >::begin(); }
  iterator end() noexcept { return std::vector< KeyValuePair >::end(); }
  const_iterator begin() const noexcept { return std::vector< KeyValuePair >::begin(); }
  const_iterator end() const noexcept { return std::vector< KeyValuePair >::end(); }
  const_iterator cbegin() const noexcept { return std::vector< KeyValuePair >::cbegin(); }
  const_iterator cend() const noexcept { return std::vector< KeyValuePair >::cend(); }

  void Add(byte_array_type const& name, byte_array_type const &value) {
    push_back({ name, value });
  }

private:

};

};
};

#endif
