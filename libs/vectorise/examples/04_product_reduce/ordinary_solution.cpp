#include<iostream>
#include<vector>

typedef std::vector< float > array_type;

float InnerProduct(array_type const &A, array_type const &B) 
{
  float ret = 0;
  
  for(std::size_t i = 0; i < A.size(); ++i) {
    float d = A[i] - B[i]
    ret += d * d;
  }
  
  return ret;
}

int main(int argc, char **argv) 
{
  std::vector< float > A, B;

  InnerProduct(A, B);
  
  return 0;  
}
