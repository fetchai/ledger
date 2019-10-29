#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

namespace fetch {
namespace crypto {

enum SignatureType : uint8_t
{
  SECP256K1_COMPRESSED   = 0x02,
  SECP256K1_COMPRESSED2  = 0x03,
  SECP256K1_UNCOMPRESSED = 0x04,
  BLS_BN256_UNCOMPRESSED = 0x20
};

constexpr bool IdentityParameterTypeDefined(uint8_t x)
{
  switch (x)
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
  switch (x)
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

}  // namespace crypto
}  // namespace fetch
