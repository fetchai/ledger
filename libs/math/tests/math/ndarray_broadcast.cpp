#include<iostream>
#include"math/ndarray.hpp"
#include"math/ndarray_iterator.hpp"
#include"math/ndarray_broadcast.hpp"

using namespace fetch::math;


int main() 
{
  NDArray< double > a = NDArray< double >::Arange(0,20,1);
  a.Reshape({1, a.size()});

  NDArray< double > b = NDArray< double >::Arange(0,20,1);
  b.Reshape({b.size(), 1});

  NDArray< double > c(a.size() * b.size());
   
  Broadcast([](double const &a, double const&b) { return a + b; }, a, b, c);

  for(std::size_t i = 0; i < c.shape(0); ++i)
  {
    for(std::size_t j = 0; j < c.shape(1); ++j)
    {
      std::cout << c({i,j}) << " ";      
    }
    std::cout << std::endl;
  }
  
       
  
  std::cout << std::endl;
  

  return 0;
}
