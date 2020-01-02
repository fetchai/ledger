//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "oef-core/comms/public_key_utils.hpp"

RSAKey::RSAKey(const EvpPublicKey &evp_pk)
  : rsa_{nullptr}
{
  ERR_load_crypto_strings();
  rsa_ = EVP_PKEY_get1_RSA(evp_pk.native_handle());
  if (rsa_ == nullptr)
  {
    throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
  }
}

std::string RSAKey::to_string() const
{
  std::string out{};

  ERR_load_crypto_strings();
  BIO *mem = BIO_new(BIO_s_mem());
  if (mem != nullptr)
  {
    FETCH_LOG_WARN(LOGGING_NAME_PK, " while creating bio mem buf");
    return out;
  }
  int status = RSA_print(mem, rsa_, 0);
  if (status != 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME_PK, " while printing RSA key to bio");
    BIO_free(mem);
    return out;
  }
  char *pk_ptr = nullptr;
  auto  len    = static_cast<unsigned long>(BIO_get_mem_data(mem, &pk_ptr));
  if (len != 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME_PK, " while getting bio char pointer");
    BIO_free(mem);
    return out;
  }
  std::string pk_str{pk_ptr, len};
  out = pk_str;
  BIO_free(mem);

  return out;
}

// TOFIX doesn't output same text as when ASCII priting PEM file
// TOFIX remove code redundency with to_string()
std::string RSAKey::to_string_base64() const
{
  std::string out{};

  ERR_load_crypto_strings();
  BIO *mem  = BIO_new(BIO_s_mem());
  BIO *b64f = BIO_new(BIO_f_base64());
  mem       = BIO_push(b64f, mem);
  BIO_set_flags(mem, BIO_FLAGS_BASE64_NO_NL);

  auto const ret = BIO_set_close(mem, BIO_CLOSE);
  (void)ret;

  if ((mem == nullptr) || (b64f == nullptr))
  {
    FETCH_LOG_WARN(LOGGING_NAME_PK, " while creating bio mem buf");
    return out;
  }
  int status = RSA_print(mem, rsa_, 0);
  if (status == 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME_PK, " while printing RSA key to bio");
    BIO_free(mem);
    return out;
  }
  char *pk_ptr = nullptr;
  auto  len    = static_cast<unsigned long>(BIO_get_mem_data(mem, &pk_ptr));
  if (len == 0u)
  {
    FETCH_LOG_WARN(LOGGING_NAME_PK, " while getting bio char pointer");
    BIO_free(mem);
    return out;
  }
  std::string pk_str{pk_ptr, len};
  out = pk_str;
  BIO_free(mem);

  return out;
}

// TOFIX inline these

std::string RSA_Modulus(const EvpPublicKey &evp_pk)
{
  const unsigned int PREFIX  = 35;
  const unsigned int SUFFIX  = 28;
  std::string        rsa_str = evp_pk.to_string();
  if (rsa_str.size() < PREFIX + SUFFIX)
  {
    return std::string{};
  }
  return std::string{rsa_str.c_str() + PREFIX, rsa_str.size() - PREFIX - SUFFIX};  // parse Modulus
}

std::string RSA_Modulus_short(const EvpPublicKey &evp_pk)
{
  const unsigned int PREFIX      = 5;
  const unsigned int NBYTES      = 23;  // equiv to ?bytes
  std::string        rsa_str_mod = RSA_Modulus(evp_pk);
  if (rsa_str_mod.size() < PREFIX + NBYTES)
  {
    return std::string{};
  }
  return std::string{rsa_str_mod.c_str() + PREFIX, NBYTES};
}

std::ostream &operator<<(std::ostream &os, const EvpPublicKey &evp_pk)
{
  return os << RSA_Modulus_short(evp_pk);
}
