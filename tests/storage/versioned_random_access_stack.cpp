#include"storage/versioned_random_access_stack.hpp"
#include"random/lfg.hpp"

#include<iostream>
#include<stack>
using namespace fetch::storage;

template< typename T >
void TestPush() {
  VersionedRandomAccessStack< T > stack;
  stack.New("versioned_random_access_stack_test_1.db", "versioned_random_access_stack_diff.db");

  auto cp1 = stack.Commit();
  stack.Push(1);
  stack.Push(2);
  stack.Push(3);    
  auto cp2 = stack.Commit();
  stack.Swap(1,2);
  stack.Push(4);
  stack.Push(5);
  stack.Set(0, 9);
  auto cp3 = stack.Commit();
  stack.Push(6);
  stack.Push(7);

  if(stack.Top() != 7) {
    std::cout << "Expected 7, but got something else" << std::endl;
    exit(-1);
  }

  if(stack.Get(0) != 9) {
    std::cout << "Expected first element to be 9, but got " << stack.Get(0) << std::endl;
    exit(-1);
  }

  if(stack.Get(1) != 3) {
    std::cout << "Expected 2nd element to be 3, but got " << stack.Get(1) << std::endl;
    exit(-1);
  }

  if(stack.Get(2) != 2) {
    std::cout << "Expected 3rd element to be 2, but got " << stack.Get(1) << std::endl;
    exit(-1);
  }

  
  
  stack.Revert(cp3);
  if(stack.Top() != 5) {
    std::cout << "Expected 5, but got something else: " << stack.Top() <<  std::endl;
    exit(-1);
  }

  if(stack.Get(0) != 9) {
    std::cout << "Expected first element to be 9, but got " << stack.Get(0) << std::endl;
    exit(-1);
  }  


  if(stack.Get(1) != 3) {
    std::cout << "Expected 2nd element to be 3, but got " << stack.Get(1) << std::endl;
    exit(-1);
  }

  if(stack.Get(2) != 2) {
    std::cout << "Expected 3rd element to be 2, but got " << stack.Get(1) << std::endl;
    exit(-1);
  }

  
  
  stack.Revert(cp2);
  if(stack.Top() != 3) {
    std::cout << "Expected 3, but got something else" << std::endl;
    exit(-1);
  }

  if(stack.Get(0) != 1) {
    std::cout << "Expected first element to be 1, but got " << stack.Get(0) << std::endl;
    exit(-1);
  }


  if(stack.Get(1) != 2) {
    std::cout << "Expected 2nd element to be 2, but got " << stack.Get(1) << std::endl;
    exit(-1);
  }

  if(stack.Get(2) != 3) {
    std::cout << "Expected 3rd element to be 3, but got " << stack.Get(1) << std::endl;
    exit(-1);
  }

  stack.Revert(cp1);
  if(!stack.empty()) {
    std::cout << "Expected empty stack but it's not empty: " << stack.size() <<  std::endl;
    exit(-1);
  }      
}

int main() {
  TestPush<uint64_t>();
  return 0;
}
