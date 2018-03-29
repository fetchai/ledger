#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"serializer/stl_types.hpp"
#include"serializer/byte_array_buffer.hpp"
#include "serializer/counter.hpp"
#include "random/lfg.hpp"

#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include<chrono>
#include<vector>
#include<iomanip>

using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace std::chrono;


fetch::random::LaggedFibonacciGenerator<> lfg;

template< typename T, std::size_t N = 256 >
void MakeString(T &str) {
  ByteArray entry;
  entry.Resize(256);
  
  for(std::size_t j=0; j < 256; ++j) {
    entry[j] = (lfg()  >> 19);      
  }
  
  str = entry;
}

template< typename T, std::size_t N = 256 >
void MakeStringVector(std::vector< T >  &vec, std::size_t size) {

  for(std::size_t i=0; i < size; ++i ){
    T s;
    MakeString(s);
    vec.push_back( s );
  }
}


std::size_t PopulateData(std::vector< uint32_t > &s ) {
  s.resize( 16 * 100000 );
  
  for(std::size_t i=0; i < s.size(); ++i) {
    s[i] = lfg();
  }
  
  return sizeof( uint32_t ) * s.size();
}


std::size_t PopulateData(std::vector< uint64_t > &s ) {
  s.resize( 16 * 100000 );
  
  for(std::size_t i=0; i < s.size(); ++i) {
    s[i] = lfg();
  }
  
  return sizeof( uint64_t ) * s.size();
}

std::size_t PopulateData(std::vector< ConstByteArray > &s ) {
  MakeStringVector(s, 100000);
  return s[0].size() * s.size();
}


std::size_t PopulateData(std::vector< ByteArray > &s ) {
  MakeStringVector(s, 100000);
  return s[0].size() * s.size();
}

std::size_t PopulateData(std::vector< std::string > &s ) {
  MakeStringVector(s, 100000);
  return s[0].size() * s.size();
}

struct Result {
  double serialization_time;
  double deserialization_time;  
  double serialization;
  double deserialization;
  double size;
};

template< typename S,  typename T, typename ... Args >
Result BenchmarkSingle( Args ... args  ) {
  Result ret;
  T data;
  std::size_t size = PopulateData( data, args... );

  S buffer;
  
  high_resolution_clock::time_point t1 = high_resolution_clock::now();    
  SizeCounter< S > counter;
  counter << data;  
  buffer.Reserve( counter.size() );    
  buffer << data;

  high_resolution_clock::time_point t2 = high_resolution_clock::now();

  T des;
  buffer.Seek(0);
  buffer >> des;

  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  duration<double> ts1 = duration_cast<duration<double>>(t2 - t1);
  duration<double> ts2 = duration_cast<duration<double>>(t3 - t2);
  ret.size = size * 1e-6;
  ret.serialization_time = ts1.count();
  ret.deserialization_time = ts2.count();    
  ret.serialization = size * 1e-6 / ts1.count();
  ret.deserialization = size * 1e-6/ ts2.count();  
  return ret;
}


#define SINGLE_BENCHMARK(serializer, type)       \
  result = BenchmarkSingle< serializer, type >();       \
  std::cout << std::setw(type_width) << #type;\
  std::cout << std::setw(width) << result.size ;  \
  std::cout << std::setw(width) << result.serialization_time ;\
  std::cout << std::setw(width) << result.deserialization_time ;\
  std::cout << std::setw(width) << result.serialization ;\
  std::cout << std::setw(width) << result.deserialization << std::endl

int main()
{
  std::size_t type_width = 35;
  std::size_t width = 12;

  std::cout << std::setw(type_width) << "Type";
  std::cout << std::setw(width) << "MBs";  
  std::cout << std::setw(width) << "Ser. time";
  std::cout << std::setw(width) << "Des. time" ;
  std::cout << std::setw(width) << "Ser. MBs" ;
  std::cout << std::setw(width) << "Des. MBs" << std::endl;

  Result result;

  SINGLE_BENCHMARK( ByteArrayBuffer, std::vector< uint32_t > );      
  SINGLE_BENCHMARK( ByteArrayBuffer, std::vector< uint64_t > );    
  SINGLE_BENCHMARK( ByteArrayBuffer, std::vector< ByteArray > );  
  SINGLE_BENCHMARK( ByteArrayBuffer, std::vector< ConstByteArray > );
  SINGLE_BENCHMARK( ByteArrayBuffer, std::vector< std::string > );      


  std::cout << std::endl;
  
  std::cout << std::setw(type_width) << "Type";
  std::cout << std::setw(width) << "MBs";  
  std::cout << std::setw(width) << "Ser. time";
  std::cout << std::setw(width) << "Des. time" ;
  std::cout << std::setw(width) << "Ser. MBs" ;
  std::cout << std::setw(width) << "Des. MBs" << std::endl;

  SINGLE_BENCHMARK( TypedByte_ArrayBuffer, std::vector< uint32_t > );   
  SINGLE_BENCHMARK( TypedByte_ArrayBuffer, std::vector< uint64_t > );    
  SINGLE_BENCHMARK( TypedByte_ArrayBuffer, std::vector< ByteArray > );  
  SINGLE_BENCHMARK( TypedByte_ArrayBuffer, std::vector< ConstByteArray > );
  SINGLE_BENCHMARK( TypedByte_ArrayBuffer, std::vector< std::string > );      
  

  /*
  std::vector< ByteArray > a,b,c;
  MakeStringVector(a, 100000);

  
  ByteArrayBuffer buffer;

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
  for(std::size_t i=0; i < b.size(); ++i) {
    if(a[i] != b[i]) {
      std::cerr << "Mismatch" << std::endl;
      exit(-1);
    }
  }
  */
  return 0;
}
