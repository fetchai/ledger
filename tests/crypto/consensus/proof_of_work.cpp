#include<iostream>
#include"chain/consensus/proof_of_work.hpp"
#include"unittest.hpp"
#include"byte_array/encoders.hpp"
using namespace fetch::consensus;
using namespace fetch::byte_array;
int main(int argc, char const **argv) {
  if(argc != 2) {
    std::cerr << "Usage " << argv[0] << " diffifulty" << std::endl;
    exit(-1);
  }
  int diff = atoi( argv[1]) ;
  
  SCENARIO("testing proof of work / double SHA") {
    ProofOfWork proof("Hello world");
    proof.SetTarget( diff );
    while( !proof() ) {
      ++proof;
    }
    std::cout << "Found proof " << std::endl;
    std::cout << ToHex(proof.digest()) << " vs " << ToHex(proof.target()) << std::endl;
    EXPECT( proof.digest() < proof.target() );

    EXPECT( false );
  };

  return 0;
}
