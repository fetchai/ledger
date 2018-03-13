#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"random/lfg.hpp"
#include"./test_aea.hpp" // TODO: (`HUT`) : delete unnecc.

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::random;

// Example of OEF code performing basic register-query functionality

int main() {

  LaggedFibonacciGenerator<> lfg;
  std::vector<std::shared_ptr<TestAEA>> testAEAs;

  for (int i = 0; i < 100; ++i) {
    testAEAs.push_back(std::make_shared<TestAEA>(lfg()));
  }

  while(1) {}
}
