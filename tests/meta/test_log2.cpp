
#include"meta/log2.hpp"

#include<iostream>

int main() {
#define TEST(X) \
  if( details::meta::Log2< (1ull << X) >::value != X ) return -1
  TEST(1);
  TEST(2);
  TEST(3);
  TEST(4);
  TEST(5);
  TEST(6);
  TEST(7);
  TEST(8);
  TEST(9);
  TEST(10);
  return 0;
}
