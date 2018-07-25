#ifndef CRYPTO_OPENSSL_PRIVATE_KEY_HPP
#define CRYPTO_OPENSSL_PRIVATE_KEY_HPP

#include "crypto/openssl_ecdsa_public_key.hpp"


namespace fetch {
namespace crypto {
namespace openssl {


template<int P_ECDSA_Curve_NID = NID_secp256k1
       , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
class ECDSAPrivateKey
{
public:
    using public_key_type = ECDSAPublicKey<P_ECDSA_Curve_NID, P_ConversionForm>;
    using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;
    static constexpr point_conversion_form_t conversionForm = P_ConversionForm;

private:
    //TODO: Keep key encrypted
    const shrd_ptr_type<EC_KEY> private_key_;
    const public_key_type public_key_;

public:

    ECDSAPrivateKey()
        : private_key_(GenerateKeyPair())
        , public_key_(ExtractPublicKey(private_key_.get()))
    {
    }


    ECDSAPrivateKey(const byte_array::ConstByteArray& key_data)
        : ECDSAPrivateKey(ECDSAPrivateKey::GetPrivateKeyAsBIGNUM(key_data))
    {
    }


    ECDSAPrivateKey(const std::string& hex_string_key_data)
        : ECDSAPrivateKey(ECDSAPrivateKey::GetPrivateKeyAsBIGNUM(hex_string_key_data))
    {
    }


    shrd_ptr_type<const EC_KEY> key() const
    {
        return private_key_;
    }


    byte_array::ByteArray KeyAsBin() const
    {
        const BIGNUM* key_as_BN = EC_KEY_get0_private_key(private_key_.get());
        if(!key_as_BN)
        {
            throw std::runtime_error("ECDSAPrivateKey::keyAsBin(): EC_KEY_get0_private_key(...) failed.");
        }

        byte_array::ByteArray key_as_bin;
        key_as_bin.Resize(static_cast<std::size_t>(BN_num_bytes(key_as_BN)));

        if (!BN_bn2bin(key_as_BN, static_cast<unsigned char *>(key_as_bin.pointer()))) {
            throw std::runtime_error("ECDSAPrivateKey::keyAsBin(...): BN_bn2bin(...) failed.");
        }

        return key_as_bin;
    }

    
    const public_key_type& publicKey() const {
        return public_key_;
    }


private:


   ECDSAPrivateKey(shrd_ptr_type<BIGNUM, del_strat_type::clearing> private_key_as_BN)
        : private_key_( ConvertPrivateKeyBN2ECKEY(private_key_as_BN.get()))
        , public_key_( DerivePublicKey(private_key_as_BN.get(), private_key_.get()))
    {
    }


    static uniq_ptr_type<BIGNUM, del_strat_type::clearing> GetPrivateKeyAsBIGNUM(const byte_array::ConstByteArray& key_data)
    {
        if (ecdsa_curve_type::privateKeySize != key_data.size())
        {
            throw std::runtime_error("ECDSAPrivateKey::GetPrivateKeyAsBIGNUM(const byte_array::ConstByteArray&): Lenght of provided byte array does not correspond to expected lenght for selected elliptic curve");
        }
        uniq_ptr_type<BIGNUM, del_strat_type::clearing> private_key_as_BN(BN_new());
        BN_bin2bn(key_data.pointer(), int(ecdsa_curve_type::privateKeySize), private_key_as_BN.get());

        if (!private_key_as_BN)
        {
            throw std::runtime_error("ECDSAPrivateKey::GetPrivateKeyAsBIGNUM(const byte_array::ConstByteArray&): BN_bin2bn(...) failed.");
        }

        return private_key_as_BN;
    }


    static uniq_ptr_type<BIGNUM, del_strat_type::clearing> GetPrivateKeyAsBIGNUM(const std::string& hex_string_key_data)
    {
        if(ecdsa_curve_type::privateKeySize*2 != hex_string_key_data.size())
        {
            throw std::runtime_error("ECDSAPrivateKey::GetPrivateKeyAsBIGNUM(const std::string&): Lenght of provided byte array does not correspond to expected lenght for priv key for selected elliptic curve");
        }

        BIGNUM *private_key_as_BN = nullptr;
        const int res = BN_hex2bn(&private_key_as_BN, hex_string_key_data.c_str());
        uniq_ptr_type<BIGNUM, del_strat_type::clearing> retval {private_key_as_BN};
        if (!res || !retval)
        {
            throw std::runtime_error("ECDSAPrivateKey::GetPrivateKeyAsBIGNUM(const std::string&): BN_hex2bn(...) failed.");
        }

        return retval;
    }


    static uniq_ptr_type<EC_KEY> ConvertPrivateKeyBN2ECKEY(const BIGNUM* private_key_as_BN)
    {
        uniq_ptr_type<EC_KEY> priv_key {EC_KEY_new_by_curve_name(ecdsa_curve_type::nid)};
        if( !EC_KEY_set_private_key(priv_key.get(), private_key_as_BN ) )
        {
            throw std::runtime_error("ECDSAPrivateKey::ConvertPrivateKeyBN2ECKEY(...): EC_KEY_set_private_key(...) failed.");
        }

        return priv_key;
    }


    static public_key_type DerivePublicKey(const BIGNUM *private_key_as_BN, EC_KEY *private_key) {
        const EC_GROUP *group = EC_KEY_get0_group(private_key);
        shrd_ptr_type<EC_POINT> public_key {EC_POINT_new(group)};
        context::Session<BN_CTX> session;

        if (!EC_POINT_mul(group, public_key.get(), private_key_as_BN, NULL, NULL, session.context().get()))
        {
            throw std::runtime_error("ECDSAPrivateKey::DerivePublicKey(...): EC_POINT_mul(...) failed.");
        }

        if(!EC_KEY_set_public_key(private_key, public_key.get()))
        {
            throw std::runtime_error("ECDSAPrivateKey::DerivePublicKey(...): EC_KEY_set_public_key(...) failed.");
        }

        return public_key_type {public_key, group, session};
    }


    static public_key_type ExtractPublicKey(const EC_KEY *private_key)
    {
        const EC_GROUP *group = EC_KEY_get0_group(private_key);
        const EC_POINT *pub_key_reference = EC_KEY_get0_public_key(private_key);

        shrd_ptr_type<EC_POINT> public_key {EC_POINT_dup(pub_key_reference, group)};
        if(!public_key)
        {
            throw std::runtime_error("ECDSAPrivateKey::ExtractPublicKey(...): EC_POINT_dup(...) failed.");
        }

        return  public_key_type {public_key, group, context::Session<BN_CTX>{}};
    }


    static uniq_ptr_type<EC_KEY> GenerateKeyPair()
    {
        uniq_ptr_type<EC_KEY> key_pair {EC_KEY_new_by_curve_name(ecdsa_curve_type::nid)};
        if(!key_pair)
        {
            throw std::runtime_error("ECDSAPrivateKey::GenerateKeyPair(): EC_KEY_new_by_curve_name(...) failed.");
        }

        if(!EC_KEY_generate_key(key_pair.get()))
        {
            throw std::runtime_error("ECDSAPrivateKey::GenerateKeyPair(): EC_KEY_generate_key(...) failed.");
        }

        return key_pair;
    }
};

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_PRIVATE_KEY_HPP

