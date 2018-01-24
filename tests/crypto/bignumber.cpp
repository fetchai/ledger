#include"crypto/bignumber.hpp"

#include"unittest.hpp"
using namespace fetch::crypto;

int main() {
  SCENARIO("testing shifting") {
    BigNumber n1(3);

    SECTION("testing elementary left shifting") {
      EXPECT( n1[0] == 3);
      n1 <<= 8;
      EXPECT( n1[0] == 0);
      EXPECT( n1[1] == 3);
    };

    SECTION("testing incrementer") {
      for(std::size_t count=0; count < (1<<20); ++count) {
        union {
          uint32_t value;
          uint8_t bytes[ sizeof(uint32_t) ];
        } c;
        
        for(std::size_t i=0; i < sizeof(uint32_t); ++i)
          c.bytes[i] = n1[i+1];
        CHECK( c.value == count );
               
        for(std::size_t i=0; i < 8; ++i) {
          CHECK( i == n1[0] );
          ++n1;
        }


      }
    };
    
  };


  

  return 0;
}
