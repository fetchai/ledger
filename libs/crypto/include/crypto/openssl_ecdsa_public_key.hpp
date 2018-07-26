#ifndef CRYPTO_OPENSSL_PUBLIC_KEY_HPP
#define CRYPTO_OPENSSL_PUBLIC_KEY_HPP

#include "core/byte_array/byte_array.hpp"
#include "crypto/openssl_common.hpp"
#include "crypto/openssl_context_session.hpp"
#include "crypto/openssl_memory.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template <int                     P_ECDSA_Curve_NID = NID_secp256k1,
          point_conversion_form_t P_ConversionForm =
              POINT_CONVERSION_UNCOMPRESSED>
class ECDSAPublicKey
{
public:
  using del_strat_type = memory::eDeleteStrategy;

  template <typename T,
            del_strat_type P_DeleteStrategy = del_strat_type::canonical>
  using shrd_ptr = memory::ossl_shared_ptr<T, P_DeleteStrategy>;

  template <typename T,
            del_strat_type P_DeleteStrategy = del_strat_type::canonical>
  using uniq_ptr_type = memory::ossl_unique_ptr<T, P_DeleteStrategy>;

  using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;

  static constexpr point_conversion_form_t conversionForm = P_ConversionForm;

private:
  const shrd_ptr<EC_POINT>         key_EC_POINT_;
  const shrd_ptr<EC_KEY>           key_EC_KEY_;
  const byte_array::ConstByteArray key_binary_;

  static byte_array::ByteArray Convert(EC_POINT *      public_key,
                                       const EC_GROUP *group,
                                       const context::Session<BN_CTX> &session)
  {

    shrd_ptr<BIGNUM> public_key_as_BN{BN_new()};
    if (!EC_POINT_point2bn(group, public_key, ECDSAPublicKey::conversionForm,
                           public_key_as_BN.get(), session.context().get()))
    {
      throw std::runtime_error(
          "ECDSAPublicKey::Convert(...) failed due to failure of the "
          "`EC_POINT_point2bn(...)` function.");
    }

    byte_array::ByteArray pub_key_as_bin;
    pub_key_as_bin.Resize(
        static_cast<std::size_t>(BN_num_bytes(public_key_as_BN.get())));

    if (!BN_bn2bin(public_key_as_BN.get(),
                   static_cast<unsigned char *>(pub_key_as_bin.pointer())))
    {
      throw std::runtime_error(
          "ECDSAPublicKey::Convert(...) failed due to failure of the "
          "`BN_bn2bin(...)` function.");
    }

    return pub_key_as_bin;
  }

  static uniq_ptr_type<EC_POINT> Convert(
      byte_array::ConstByteArray const &key_data)
  {
    shrd_ptr<BIGNUM> pub_key_as_BN{BN_new()};
    if (!BN_bin2bn(static_cast<const unsigned char *>(key_data.pointer()),
                   int(key_data.size()), pub_key_as_BN.get()))
    {
      throw std::runtime_error(
          "ECDSAPublicKey::ConvertToECPOINT(...): BN_bin2bn(...) failed.");
    }

    uniq_ptr_type<const EC_GROUP> group(
        EC_GROUP_new_by_curve_name(ecdsa_curve_type::nid));
    uniq_ptr_type<EC_POINT>  public_key{EC_POINT_new(group.get())};
    context::Session<BN_CTX> session;

    if (!EC_POINT_bn2point(group.get(), pub_key_as_BN.get(), public_key.get(),
                           session.context().get()))
    {
      throw std::runtime_error(
          "ECDSAPublicKey::ConvertToECPOINT(...): EC_POINT_bn2point(...) "
          "failed.");
    }

    return public_key;
  }

  static uniq_ptr_type<EC_KEY> ConvertToECKEY(const EC_POINT *key_EC_POINT)
  {
    uniq_ptr_type<EC_KEY> key{EC_KEY_new_by_curve_name(ecdsa_curve_type::nid)};
    // TODO: setting conv. form might not be really necessary (stuff works
    // without it)
    EC_KEY_set_conv_form(key.get(), ECDSAPublicKey::conversionForm);

    if (!EC_KEY_set_public_key(key.get(), key_EC_POINT))
    {
      throw std::runtime_error(
          "ECDSAPublicKey::ConvertToECKEY(...): EC_KEY_set_public_key(...) "
          "failed.");
    }

    return key;
  }

public:
  ECDSAPublicKey(shrd_ptr<EC_POINT> public_key, const EC_GROUP *group,
                 const context::Session<BN_CTX> &session)
      : key_EC_POINT_{public_key}
      , key_EC_KEY_{ConvertToECKEY(public_key.get())}
      , key_binary_{Convert(public_key.get(), group, session)}
  {}

  ECDSAPublicKey(const byte_array::ConstByteArray &key_data)
      : key_EC_POINT_{Convert(key_data)}
      , key_EC_KEY_{ConvertToECKEY(key_EC_POINT_)}
      , key_binary_{key_data}
  {}

  shrd_ptr<const EC_POINT> keyAsEC_POINT() const { return key_EC_POINT_; }

  shrd_ptr<const EC_KEY> key() const { return key_EC_KEY_; }

  const byte_array::ConstByteArray &keyAsBin() const { return key_binary_; }
};

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch

#endif  // CRYPTO_OPENSSL_PUBLIC_KEY_HPP
