#include"storage/variant_stack.hpp"
#include"random/lfg.hpp"

#include<iostream>
#include<stack>
using namespace fetch::storage;

template< typename T >
void SimpleTest(std::size_t N = 100) {
  VariantStack stack;
  std::stack< T > reference;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  
  stack.New("variant_stack_test_1.db");
  if(! stack.empty() ) {
    std::cerr << "expected that a new stack should be empty" << std::endl;
    exit(-1);
  }
  
  for(std::size_t i=0; i < N; ++i) {
    if(stack.size() != i) {
      std::cerr << "expected stack to be size " << i << ", but " << stack.size() << " found" << std::endl;
      exit(-1);
    }
    
    uint64_t val = lfg();
    reference.push(val);
    stack.Push(val);
    uint64_t ref;
    stack.Top(ref);

    if(ref != val) {
      std::cout << "top and last pushed value mismatch " << val << " " << ref << std::endl;
      exit(-1);
    }
  }


  for(std::size_t i=0; i < N; ++i) {
    uint64_t top, ref;
    stack.Top(top);
    ref = reference.top();

    reference.pop();
    stack.Pop();

    if(ref != top) {
      std::cerr << "Top different from reference" << std::endl;
    }
  }
  
  if(! stack.empty() ) {
    std::cerr << "expected that a new stack should be empty (II)" << std::endl;
    exit(-1);
  }

}

int main() {
  SimpleTest<uint64_t>();
  return 0;
}
