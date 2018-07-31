#include"vectorise/memory/shared_array.hpp"
#include<iostream>
#include<vector>

typedef fetch::memory::SharedArray< float > array_type;
typedef typename array_type::vector_register_type vector_type;

float InnerProduct(array_type const &A) 
{
  float ret = 0;
  
  ret = A.in_parallel().SumReduce([](vector_type const &a,
      vector_type const &b) {
      vector_type d = a - b;
      return d * d;      
    }, B);
  
  return ret;
}

int main(int argc, char **argv) 
{
  array_type A, B;

  InnerProduct(A, B);
  
  return 0;  
}
