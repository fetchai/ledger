#pragma once

namespace fetch
{
namespace crypto
{

enum SignatureType : uint8_t
{
  SECP256K1_COMPRESSED     = 0x02,  
  SECP256K1_COMPRESSED2    = 0x03,
  SECP256K1_UNCOMPRESSED   = 0x04,
  BLS_BN256_UNCOMPRESSED   = 0x20
};

constexpr bool IdentityParameterTypeDefined(uint8_t x)
{
  switch(x)
  {
  case SECP256K1_COMPRESSED:
  case SECP256K1_COMPRESSED2:
  case SECP256K1_UNCOMPRESSED:
  case BLS_BN256_UNCOMPRESSED:
    return true;
  }

  return false;
}

constexpr bool TestIdentityParameterSize(uint8_t x, uint64_t s)
{
  switch(x)
  {
  case SECP256K1_COMPRESSED:
  case SECP256K1_COMPRESSED2:
    return s == 32;
  case SECP256K1_UNCOMPRESSED:
    return s == 64;
  case BLS_BN256_UNCOMPRESSED:
    return true;
  }

  return false;
}

}
}