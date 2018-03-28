#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"serializer/stl_types.hpp"
#include"serializer/byte_array_buffer.hpp"
#include "serializer/counter.hpp"
#include "random/lfg.hpp"
#include<chrono>
using namespace fetch::serializers;
using namespace fetch::byte_array;

fetch::random::LaggedFibonacciGenerator<> lfg;


void TestSerializationSpeed() 
{
  std::vector< ByteArray > a,b,c;
  for(std::size_t i=0; i < 100000; ++i ){
    ByteArray entry;
    entry.Resize(256);

    for(std::size_t j=0; j < 256; ++j) {
      entry[j] = (lfg()  >> 19);      
    }

    a.push_back(entry);
//    a.push_back("W18rRzylnPc35jkyCWkObNT0LWkU4uswXkpZkRzOQFWG5sy9Gb9ji9yP7Wzsc5NC4y5RJuQN9Og29RrPJF2ZMhwnrIL3Z4AvkHkq");    
//    if(i % 3) c.push_back(entry);    
  }
  
  
  ByteArrayBuffer buffer;

  using namespace std::chrono;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();  
  std::sort(a.begin(), a.end());
  
  SizeCounter< ByteArrayBuffer > counter;
  
  counter << a;  
  buffer.Reserve( counter.size() );    
  buffer << a;
  
  high_resolution_clock::time_point t2 = high_resolution_clock::now();  
  buffer.Seek(0);
  buffer >> b;
  std::sort(c.begin(), c.end());
  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  duration<double> ts1 = duration_cast<duration<double>>(t2 - t1);
  duration<double> ts2 = duration_cast<duration<double>>(t3 - t2);
  std::cout << "It took " << ts1.count() << " seconds.";
  std::cout << std::endl;    
  std::cout << "It took " << ts2.count() << " seconds.";  
  std::cout << std::endl;  
}


int main() {
  TestSerializationSpeed() ;
  
  return 0;
}
