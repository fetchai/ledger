#ifndef CRYPTO_ECDSA_HPP
#define CRYPTO_ECDSA_HPP
#include <crypto/hash.hpp>
#include <core/assert.hpp>
#include <crypto/prover.hpp>
#include <crypto/sha256.hpp>
#include "crypto/openssl_memory.hpp"

#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include <memory>

namespace fetch {
namespace crypto {

class ECDSASigner : public Prover {
    void FreeMemory() {
        if (key_ != nullptr) EC_KEY_free(key_);
        if (group_ != nullptr) EC_GROUP_clear_free(group_);
        key_ = nullptr;
        group_ = nullptr;
    }

    public:
    enum { PRIVATE_KEY_SIZE = 32 };

    void Load(byte_array_type const &private_key) override {
        SetPrivateKey(private_key);
    }

    void SetPrivateKey(byte_array_type const &private_key) {
        FreeMemory();

        // Setting the private key
        private_key_ = private_key;
        key_ = EC_KEY_new_by_curve_name(NID_secp256k1);

        BIGNUM *private_as_bn = BN_new();
        assert(private_key.size() == 32);

        BN_bin2bn(private_key.pointer(), int(private_key.size()), private_as_bn);
        EC_KEY_set_private_key(key_, private_as_bn);

        // Deriving the public key
        BN_CTX *ctx = BN_CTX_new();
        BN_CTX_start(ctx);

        // Should be freed
        EC_GROUP const *group = EC_KEY_get0_group(key_);
        EC_POINT *public_key = EC_POINT_new(group);
        EC_POINT_mul(group, public_key, private_as_bn, NULL, NULL, ctx);
        EC_KEY_set_public_key(key_, public_key);

        // Getting public key
        //   EC_KEY_set_conv_form(key_, POINT_CONVERSION_COMPRESSED);
        public_key_.Resize(std::size_t(i2o_ECPublicKey(key_, NULL)));

        //    assert( public_key_.size() == 64 );
        uint8_t *pub_copy = public_key_.pointer();
        if (std::size_t(i2o_ECPublicKey(key_, &pub_copy)) != public_key_.size()) {
          TODO_FAIL("Unable to decode public key");
        }

        EC_POINT_free(public_key);

        BN_CTX_end(ctx);
        BN_CTX_free(ctx);

        BN_clear_free(private_as_bn);
    }

    void GenerateKeys() {
        FreeMemory();

        const int set_group_success = 1;
        const int gen_success = 1;

        key_ = EC_KEY_new();
        if (key_ == nullptr) {
          TODO_FAIL("Failed to create new EC Key\n");
        }

        group_ = EC_GROUP_new_by_curve_name(NID_secp192k1);
        if (group_ == nullptr) {
          TODO_FAIL("Failed to create new EC Group\n");
        }

        int set_group_status = EC_KEY_set_group(key_, group_);

        if (set_group_success != set_group_status) {
          TODO_FAIL("Failed to set group for EC Key\n");
        }

        int gen_status = EC_KEY_generate_key(key_);
        if (gen_success != gen_status) {
          TODO_FAIL("Failed to generate EC Key\n");
        }
    }

    bool Sign(byte_array_type const &text) final override {
        hash_ = Hash<SHA256>(text);
        signature_.Resize(std::size_t(ECDSA_size(key_)));
        uint32_t len = uint32_t(signature_.size());
        int ret = ECDSA_sign(0, hash_.pointer(), int(hash_.size()),
                             signature_.pointer(), &len, key_);
        return (ret != 1);
    }

    void Verify() {
        /*
        const int verify_success = 1;
        int verify_status = ECDSA_do_verify(hash_.pointer(), hash_.size(),
        signature_, key_);

        if (verify_success != verify_status)   {
          TODO_FAIL("Failed to verify EC Signature\n");
        }
        printf("Verifed EC Signature\n");
        */
    }

    byte_array_type document_hash() final override { return hash_; }

    byte_array_type signature() final override { return signature_; }
    byte_array_type public_key() { return public_key_; }
    byte_array_type private_key() { return private_key_; }

    private:
    byte_array_type hash_;
    byte_array_type private_key_;
    byte_array_type public_key_;
    EC_KEY *key_ = nullptr;
    byte_array_type signature_;
    EC_GROUP *group_ = nullptr;

    openssl::memory::ossl_unique_ptr<EC_KEY> bbp_ec_new_keypair(const uint8_t *priv_bytes) {
        openssl::memory::ossl_unique_ptr<EC_KEY> key;

        BIGNUM *priv;
        BN_CTX *ctx;
        const EC_GROUP *group;
        EC_POINT *pub;

        /* init empty OpenSSL EC keypair */

        key.reset(EC_KEY_new_by_curve_name(NID_secp256k1));

        /* set private key through BIGNUM */

        priv = BN_new();
        BN_bin2bn(priv_bytes, 32, priv);
        EC_KEY_set_private_key(key.get(), priv);

        /* derive public key from private key and group */

        ctx = BN_CTX_new();
        BN_CTX_start(ctx);

        group = EC_KEY_get0_group(key.get());
        pub = EC_POINT_new(group);
        EC_POINT_mul(group, pub, priv, NULL, NULL, ctx);
        EC_KEY_set_public_key(key.get(), pub);

        /* release resources */

        EC_POINT_free(pub);
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
        BN_clear_free(priv);

        return key;
    }

    EC_KEY *bbp_ec_new_pubkey(const uint8_t *pub_bytes, int pub_len) {
        EC_KEY *key;
        const uint8_t *pub_bytes_copy;

        key = EC_KEY_new_by_curve_name(NID_secp256k1);
        pub_bytes_copy = pub_bytes;
        o2i_ECPublicKey(&key, &pub_bytes_copy, pub_len);

        return key;
    }
    };
}
}
#endif
