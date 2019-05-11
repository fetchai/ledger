#pragma once
#include <unordered_map>

namespace fetch
{
namespace vm
{

class PointerRegister {
public:
  template< typename T > 
  void Set(T* val)
  {
    pointers_[std::type_index(typeid(T))] = static_cast< void* >(val);
  }  

  template< typename T > 
  T *Get()
  {
    if(pointers_.find( std::type_index(typeid(T) )) == pointers_.end())
    {
      return nullptr;
    }

    return static_cast< T* >( pointers_[std::type_index(typeid(T) )] );
  }
private:
  std::unordered_map< std::type_index, void* > pointers_;
};

}
}