#pragma once

#include "crypto/openssl_ecdsa_private_key.hpp"
#include "crypto/openssl_memory.hpp"
#include "crypto/sha256.hpp"

#include <openssl/ecdsa.h>

namespace fetch {
namespace crypto {

namespace {
constexpr int ignored_type_val{0};
}

template <typename T_Hasher = SHA256, int P_ECDSA_Curve_NID = NID_secp256k1,
          point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
byte_array::ByteArray ecdsa_sign(
    openssl::ECDSAPrivateKey<P_ECDSA_Curve_NID, P_ConversionForm> const &private_key,
    byte_array::ConstByteArray const &data_to_sign, byte_array::ByteArray *hash = nullptr)
{

  T_Hasher hasher;
  hasher.Reset();
  hasher.Update(data_to_sign);
  hasher.Final();

  const auto calc_hash = hasher.digest();

  byte_array::ByteArray signature;
  signature.Resize(static_cast<std::size_t>(ECDSA_size(private_key.key().get())));

  unsigned int acquired_signature_length{static_cast<unsigned int>(signature.size())};

  if (!ECDSA_sign(ignored_type_val, static_cast<const unsigned char *>(calc_hash.pointer()),
                  int(calc_hash.size()), static_cast<unsigned char *>(signature.pointer()),
                  &acquired_signature_length, const_cast<EC_KEY *>(private_key.key().get())))
  {

    throw std::runtime_error("ecdsa_sign(): ECDSA_sign(...) failed.");
  }

  if (static_cast<unsigned int>(signature.size()) < acquired_signature_length)
  {
    throw std::runtime_error("ecdsa_sign(): unexpected length of the signature.");
  }

  signature.Resize(acquired_signature_length);

  if (hash)
  {
    *hash = calc_hash;
  }

  return signature;
}

template <int                     P_ECDSA_Curve_NID = NID_secp256k1,
          point_conversion_form_t P_ConversionForm  = POINT_CONVERSION_UNCOMPRESSED>
bool ecdsa_verify_hash(
    openssl::ECDSAPublicKey<P_ECDSA_Curve_NID, P_ConversionForm> const &public_key,
    byte_array::ConstByteArray const &hash_to_verify, byte_array::ByteArray const &signature)
{

  const int res = ECDSA_verify(
      ignored_type_val, static_cast<const unsigned char *>(hash_to_verify.pointer()),
      static_cast<int>(hash_to_verify.size()),
      static_cast<const unsigned char *>(signature.pointer()), static_cast<int>(signature.size()),
      const_cast<EC_KEY *>(public_key.key().get()));

  switch (res)
  {
  case 1:
    return true;

  case 0:
    return false;

  case -1:
  default:
    throw std::runtime_error("ecdsa_verify(): ECDSA_verify(...) failed.");
  }
}

template <typename T_Hasher = SHA256, int P_ECDSA_Curve_NID = NID_secp256k1,
          point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
bool ecdsa_verify(openssl::ECDSAPublicKey<P_ECDSA_Curve_NID, P_ConversionForm> const &public_key,
                  byte_array::ConstByteArray const &data_to_verify,
                  byte_array::ByteArray const &     signature)
{

  T_Hasher hasher;
  hasher.Reset();
  hasher.Update(data_to_verify);
  hasher.Final();

  const auto hash = hasher.digest();

  return ecdsa_verify_hash(public_key, hash, signature);
}

}  // namespace crypto
}  // namespace fetch
