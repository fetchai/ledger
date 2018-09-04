#pragma once 

#include <vector>
#include <typeinfo>

namespace fetch
{
namespace vm
{

namespace details {

template < typename... Args>  
struct ArgumentsToList ;

template <typename T,  typename... Args>  
struct ArgumentsToList<T, Args...> {

  static void AppendTo(std::vector< std::type_index > &list)
  {
    list.push_back( std::type_index(typeid( std::declval< T >() )) ) ;
    ArgumentsToList< Args ... >::AppendTo( list );
  }
};

template <typename T >  
struct ArgumentsToList<T> {

  static void AppendTo(std::vector< std::type_index > &list)
  {
    list.push_back( std::type_index(typeid( std::declval< T >() )) ) ;
  }

};

template < >  
struct ArgumentsToList<> {
  static void AppendTo(std::vector< std::type_index > &list)
  {

  }
};

}
}
}