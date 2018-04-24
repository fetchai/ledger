#include<iostream>
#include<random/lfg.hpp>
#include<vector>
#include<cmath>
#include"bit_statistics.hpp"


int main() {
  BitStatistics<> bst;  

  int ret = 0;
  if(!bst.TestAccuracy(1000000, 0.002)) 
    ret = -1;

  for(auto &a : bst.GetProbabilities())  {
    std::cout << a << " ";
  }
  std::cout << std::endl;

  return ret;
}
