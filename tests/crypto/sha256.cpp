#include<iostream>
#include"crypto/sha256.hpp"
#include"crypto/hash.hpp"
#include"byte_array/encoders.hpp"
using namespace fetch;
using namespace fetch::crypto;

#include"unittest.hpp"

typedef byte_array::ByteArray byte_array_type;
int main() {
  
  SCENARIO("The SHA256 implmentation differs from other libraries") {
    
    auto hash = [](byte_array_type const&s) {
      return byte_array::ToHex( Hash< crypto::SHA256 >(s) );
    };

    byte_array_type input = "Hello world";        
    DETAILED_EXPECT( hash(input) == "64ec88ca00b268e5ba1a35678a1b5316d212f4f366b2477232534a8aeca37f3c" ) {
      CAPTURE( Hash< crypto::SHA256 >(input) );
    };

    input = "some RandSom byte_array!! With !@#$%^&*() Symbols!";
    DETAILED_EXPECT( hash(input) == "3d4e08bae43f19e146065b7de2027f9a611035ae138a4ac1978f03cf43b61029" ) {
      CAPTURE( input );      
      CAPTURE( Hash< crypto::SHA256 >(input) );
    };    
    

  };
  
}
