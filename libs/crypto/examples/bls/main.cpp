#include "crypto/bls.hpp"
#include <iostream>
using namespace fetch::crypto;

int main()
{
bls::init();
  std::cout << "Running BLS signature" << std::endl;
  BLSSigner signer;
  signer.GenerateKeys(); 

  BLSVerifier verifier{signer.identity()};
  auto sig = signer.Sign("Hello world");

  if(verifier.Verify("Hello world", sig))
  {
    std::cout << "Signature verified." << std::endl;
  }
  return 0;
}