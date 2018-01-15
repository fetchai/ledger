#include"storage/random_access_stack.hpp"
#include"random/lfg.hpp"

#include<iostream>
#include<stack>
using namespace fetch::storage;

template< typename T >
void SimpleTest(std::size_t N = 100) {
  RandomAccessStack< T > stack;
  std::stack< T > reference;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  
  stack.New("random_access_stack_test_1.db");
  for(std::size_t i=0; i < N; ++i) {
    uint64_t val = lfg();
    reference.push(val);
    stack.Push(val);
  }

  for(std::size_t i=0; i < N; ++i) {
    uint64_t a = reference.top();
    uint64_t b = stack.Top();
    std::cout << a << " " << b << std::endl;
    if(a != b) {
      std::cerr << "failed as " << a << " != " << b << std::endl;
      exit(-1);
    }
    reference.pop();
    stack.Pop();
    if(reference.size()  != stack.size()) {
      std::cerr << "size mismatch" << std::endl;
      exit(-1);
    }
  }
}


int main() {
  SimpleTest<uint64_t>();
  return 0;
}
