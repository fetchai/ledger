#include<iostream>
#include"crypto/sha256.hpp"
#include"crypto/hash.hpp"
#include"byte_array/encoders.hpp"
using namespace fetch;
using namespace fetch::crypto;

int main() {
  std::cout << byte_array::ToHex( Hash< crypto::SHA256 >( "hello world" ) ) << std::endl;
  return 0;
}
