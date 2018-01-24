#ifndef CHAIN_BLOCK_HPP
#define CHAIN_BLOCK_HPP
namespace fetch {
namespace chain {
  
template< typename B, typename P, typename H >
class BasicBlock {
public:
  typedef B body_type;
  typedef P proof_type;
  typedef H hasher_type;
  typedef typename proof_type::header_type header_type;
  
  body_type const& SetBody(body_type const &body) {
    body_ = body;
    serializers::ByteArrayBuffer buf;

    buf << body;
    hasher_type hash;
    hash.Reset();
    hash.Update( buf.data());
    hash.Final();   
    proof_.SetHeader( hash.digest() );

    assert( hash.digest() == header() );    

    return body_;
  }

  header_type const& header() const { return proof_.header(); }  
  proof_type const & proof() const { return proof_; }
  proof_type & proof() { return proof_; }  
  body_type const & body() const { return body_; }  
private:
  body_type body_;
  proof_type proof_;    
};
  
};
};
#endif
