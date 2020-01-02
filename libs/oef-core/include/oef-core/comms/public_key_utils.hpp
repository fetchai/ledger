#pragma once
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

#include "logging/logging.hpp"
#include "network/fetch_asio.hpp"

#include <cerrno>  // for fopen
#include <cstdio>  // for fopen
#include <exception>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

// both can be used separately
// using PublicKey  = class EvpPublicKey;
using PublicKey = std::string;

static constexpr char const *LOGGING_NAME_PK = "PublicKeyUtils";

class X509CertP
{
public:
  explicit X509CertP(SSL *conn)
    : cert_{nullptr}
  {
    ERR_load_crypto_strings();  // TOFIX
    cert_ = SSL_get_peer_certificate(conn);
    if (cert_ == nullptr)
    {
      throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
    }
  }

  explicit operator X509 *() const
  {
    return cert_;
  }

  X509 *native_handle() const
  {
    return cert_;
  }

  X509CertP(const X509CertP &)  = delete;
  X509CertP(const X509CertP &&) = delete;
  X509CertP &operator=(const X509CertP &) = delete;
  X509CertP &operator=(const X509CertP &&) = delete;

  ~X509CertP()
  {
    X509_free(cert_);
  }

private:
  X509 *cert_;
};

class EvpPublicKey;

class RSAKey
{
public:
  explicit RSAKey(const EvpPublicKey &evp_pk);

  explicit operator RSA *() const
  {
    return rsa_;
  }

  RSA *native_handle() const
  {
    return rsa_;
  }

  std::string to_string() const;

  // TOFIX doesn't output same text as when ASCII priting PEM file
  // TOFIX remove code redundency with to_string()
  std::string to_string_base64() const;

  RSAKey(const RSAKey &)  = delete;
  RSAKey(const RSAKey &&) = delete;
  RSAKey &operator=(const RSAKey &) = delete;
  RSAKey &operator=(const RSAKey &&) = delete;

  ~RSAKey()
  {
    RSA_free(rsa_);
  }

private:
  RSA *rsa_;
};

class EvpPublicKey
{
public:
  explicit EvpPublicKey(X509CertP &cert)
    : evp_pk_{nullptr}
  {
    ERR_load_crypto_strings();
    evp_pk_ = X509_get_pubkey(cert.native_handle());
    if (evp_pk_ == nullptr)
    {
      throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
    }
  }

  explicit EvpPublicKey(const std::string &pem_file_path)
    : evp_pk_{nullptr}
  {
    FILE *fp = fopen(pem_file_path.c_str(), "r");
    if (fp == nullptr)
    {
      throw std::runtime_error(strerror(errno));
    }
    ERR_load_crypto_strings();
    evp_pk_ = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    if (evp_pk_ == nullptr)
    {
      throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
    }
  }

  bool operator==(const EvpPublicKey &other) const
  {
    return EVP_PKEY_cmp(evp_pk_, static_cast<EVP_PKEY const *>(other)) == 1;
  }

  bool operator<(const EvpPublicKey &other) const
  {
    // TOFIX which one to use?
    return to_string() < other.to_string();  // TOFIX
    // return !(*this == other); // TOFIX
  }

  explicit operator EVP_PKEY const *() const
  {
    return evp_pk_;
  }

  EVP_PKEY *native_handle() const
  {
    return evp_pk_;
  }

  std::string to_string() const
  {
    std::string out{};

    int type = EVP_PKEY_base_id(evp_pk_);
    switch (type)
    {
    // RSA
    case EVP_PKEY_RSA:
    case EVP_PKEY_RSA2:
    {
      try
      {
        RSAKey rsa{*this};
        out = rsa.to_string();
      }
      catch (std::exception const &e)
      {
        FETCH_LOG_WARN(LOGGING_NAME_PK, " error getting rsa key");
      }
      break;
    }

    // ECDSA TODO()

    // not supported
    default:
      break;
    }

    return out;
  }

  // TOFIX doesn't output same text as when ASCII priting PEM file
  // TOFIX remove code redundency with to_string()
  std::string to_string_base64() const
  {
    std::string out{};

    int type = EVP_PKEY_base_id(evp_pk_);
    switch (type)
    {
    // RSA
    case EVP_PKEY_RSA:
    case EVP_PKEY_RSA2:
    {
      try
      {
        RSAKey rsa{*this};
        out = rsa.to_string_base64();
      }
      catch (std::exception const &e)
      {
        FETCH_LOG_WARN(LOGGING_NAME_PK, " error getting rsa key");
      }
      break;
    }

    // ECDSA TODO()

    // not supported
    default:
      break;
    }

    return out;
  }

  explicit operator std::string() const
  {
    return to_string();
  }

  // TOFIX use copy swap idiom

  EvpPublicKey(const EvpPublicKey &original)
  {
    evp_pk_ = original.evp_pk_;
    if (EVP_PKEY_up_ref(evp_pk_) == 0)
    {
      throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
    }
  }

  EvpPublicKey(const EvpPublicKey &&original)
  {
    evp_pk_ = original.evp_pk_;
    if (EVP_PKEY_up_ref(evp_pk_) == 0)
    {
      throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
    }
  }

  EvpPublicKey &operator=(const EvpPublicKey &original)
  {
    evp_pk_ = original.evp_pk_;
    if (EVP_PKEY_up_ref(evp_pk_) == 0)
    {
      throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
    }
    return *this;
  }

  EvpPublicKey &operator=(const EvpPublicKey &&original)
  {
    evp_pk_ = original.evp_pk_;
    if (EVP_PKEY_up_ref(evp_pk_) == 0)
    {
      throw std::runtime_error(ERR_error_string(ERR_get_error(), nullptr));
    }
    return *this;
  }

  ~EvpPublicKey()
  {
    EVP_PKEY_free(evp_pk_);
  }

private:
  EVP_PKEY *evp_pk_;
};

// TOFIX inline these

std::string   RSA_Modulus(const EvpPublicKey &evp_pk);
std::string   RSA_Modulus_short(const EvpPublicKey &evp_pk);
std::ostream &operator<<(std::ostream &os, const EvpPublicKey &evp_pk);
