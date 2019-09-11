#pragma once

#include "core/logging.hpp"
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
    : cert_{NULL}
  {
    ERR_load_crypto_strings();  // TOFIX
    cert_ = SSL_get_peer_certificate(conn);
    if (!cert_)
      throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
  }

  operator X509 *() const
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

  operator RSA *() const
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
    : evp_pk_{NULL}
  {
    ERR_load_crypto_strings();
    evp_pk_ = X509_get_pubkey(cert.native_handle());
    if (!evp_pk_)
      throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
  }

  explicit EvpPublicKey(std::string pem_file_path)
    : evp_pk_{NULL}
  {
    FILE *fp = fopen(pem_file_path.c_str(), "r");
    if (!fp)
      throw std::runtime_error(strerror(errno));
    ERR_load_crypto_strings();
    evp_pk_ = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);
    if (!evp_pk_)
      throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
  }

  bool operator==(const EvpPublicKey &other) const
  {
    return EVP_PKEY_cmp(evp_pk_, other) == 1;
  }

  bool operator<(const EvpPublicKey &other) const
  {
    // TOFIX which one to use?
    return to_string() < other.to_string();  // TOFIX
    // return !(*this == other); // TOFIX
  }

  operator EVP_PKEY *() const
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
      catch (std::exception &e)
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
      catch (std::exception &e)
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

  operator std::string() const
  {
    return to_string();
  }

  // TOFIX use copy swap idiom

  EvpPublicKey(const EvpPublicKey &original)
  {
    evp_pk_ = original.evp_pk_;
    if (!EVP_PKEY_up_ref(evp_pk_))
      throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
  }

  EvpPublicKey(const EvpPublicKey &&original)
  {
    evp_pk_ = original.evp_pk_;
    if (!EVP_PKEY_up_ref(evp_pk_))
      throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
  }

  EvpPublicKey &operator=(const EvpPublicKey &original)
  {
    evp_pk_ = original.evp_pk_;
    if (!EVP_PKEY_up_ref(evp_pk_))
      throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
    return *this;
  }

  EvpPublicKey &operator=(const EvpPublicKey &&original)
  {
    evp_pk_ = original.evp_pk_;
    if (!EVP_PKEY_up_ref(evp_pk_))
      throw std::runtime_error(ERR_error_string(ERR_get_error(), NULL));
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
