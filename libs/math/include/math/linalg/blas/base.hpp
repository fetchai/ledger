#pragma once


namespace fetch
{
namespace math 
{
namespace linalg 
{


template< typename T, uint64_t I >
class Blas 
{
public:
	template<typename ...Args >
  void operator()(Args ...args) 
  {
//  	static_assert(true, "BLAS specialisation does not exist.");
  }
};

template<>
class Blas< double, Computes( _A * _B + _alpha * _C ) > 
{
public:
  void operator()() 
  {
    std::cout << "You've just got A * B + C" << std::endl;
  }  
};

}
}
}