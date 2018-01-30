#include"crypto/ecdsa.hpp"
#include"byte_array/encoders.hpp"
#include<iostream>
using namespace fetch::crypto;

int main() {
  ECDSASigner sig;
  fetch::byte_array::ByteArray key = {
    0x16, 0x26, 0x07, 0x83, 0xe4, 0x0b, 0x16, 0x73,
    0x16, 0x73, 0x62, 0x2a, 0xc8, 0xa5, 0xb0, 0x45,
    0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7, 0x27, 0xf3,
    0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42
  };
  sig.SetPrivateKey( key );
  sig.Sign( "Hello world" );
  std::cout << "from ecdsa import VerifyingKey" << std::endl;
  std::cout << "message = b\"Hello world\"" << std::endl;
  std::cout << "# " << sig.public_key().size() << " " <<  sig.private_key().size() << " ";
  std::cout << sig.signature().size() << std::endl;
  std::cout << "public_key = \"" << fetch::byte_array::ToHex(sig.public_key().SubArray(1) ) << "\"" << std::endl;
                                                                                          std::cout << "sig = \"" << fetch::byte_array::ToHex(sig.signature()) << "\"" << std::endl;
                                                                                                    std::cout << "vk = VerifyingKey.from_string(public_key.decode(\"hex\"), curve=ecdsa.SECP256k1)" << std::endl;
  std::cout << "vk.verify(sig.decode(\"hex\"), message) # True" << std::endl;
  return 0;
}
