#include "crypto/sha256.hpp"
#include "core/byte_array/encoders.hpp"
#include "crypto/hash.hpp"
#include <iostream>
using namespace fetch;
using namespace fetch::crypto;

int main()
{
  std::cout << byte_array::ToHex(Hash<crypto::SHA256>("hello world")) << std::endl;
  return 0;
}
