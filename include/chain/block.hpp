#ifndef CHAIN_BLOCK_HPP
#define CHAIN_BLOCK_HPP
namespace fetch {
namespace chain {

template< typename B >
class BasicBlock {
public:
  typedef B body_type;

  body_type const& set_body(body_type const &body) {
    body_ = body;
    serializers::ByteArrayBuffer buf;
    buf << body;
    hasher_type hash;
    hash.Reset();
    hash.Update( buf.data());
    digest_ = hash.digest();
    return body_;
  }
  

  digest_type const & digest() const { return proof_; }  
  proof_type const & proof() const { return proof_; }
private:
  body_type body_;
  digest_type digest_;
  proof_type proof_;    
};
  
};
};
#endif
