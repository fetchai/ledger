#ifndef HTTP_HEADER_HPP
#define HTTP_HEADER_HPP
#include<vector>
namespace fetch {
namespace http {

struct HeaderEntry
{
  std::string name;
  std::string value;
};

  
class Header : private std::vector< HeaderEntry >
{
public:
  typedef typename std::vector< HeaderEntry >::iterator iterator;
  typedef typename std::vector< HeaderEntry >::const_iterator const_iterator;
  void Add(std::string const& name, std::string const &value) {
    push_back({name, value });
  }
  
  iterator begin() noexcept { return std::vector< HeaderEntry >::begin(); }
  iterator end() noexcept { return std::vector< HeaderEntry >::end(); }
  const_iterator begin() const noexcept { return std::vector< HeaderEntry >::begin(); }
  const_iterator end() const noexcept { return std::vector< HeaderEntry >::end(); }
  const_iterator cbegin() const noexcept { return std::vector< HeaderEntry >::cbegin(); }
  const_iterator cend() const noexcept { return std::vector< HeaderEntry >::cend(); }      
};

};
};



#endif 
