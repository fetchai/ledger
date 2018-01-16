#define FETCH_TESTING_ENABLED
#include<iostream>
#include<random/lcg.hpp>
#include<memory/shared_array.hpp>
#include<thread>
#include<vector>
using namespace fetch::memory;

typedef SharedArray< uint16_t > array_type;

std::vector< std::thread* > threads;

void test( array_type q ) {
  std::this_thread::sleep_for( std::chrono::microseconds( q[0] ) );
}

int main() {
  array_type xx(2), yy(3), zz(2);
  yy[0] = zz[0] = xx[0] = 1;
  xx[1] =1;
  for(std::size_t j=0; j < 100000; ++j) {
    for(std::size_t i=0 ; i < 10; ++i ) {
      threads.push_back( new std::thread([&xx, &yy, &zz]() mutable {
            test( zz );                  
            test( xx );
            test( yy );
            test( zz );           
          }) );
    }
    for(auto &t: threads) {
      t->join();
      delete t;
    }
    threads.clear();
  }
  return 0;
}
  
