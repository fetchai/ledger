#ifndef CHAIN_BLOCK_HPP
#define CHAIN_BLOCK_HPP
namespace fetch {
namespace chain {
  
template< typename B, typename P, typename H, typename M >
class BasicBlock {
public:
  typedef B body_type;
  typedef P proof_type;
  typedef H hasher_type;
  typedef M meta_type;
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
  
  meta_type& meta_data() { return meta_data_; }  
  meta_type const& meta_data() const { return meta_data_; }
  header_type const& header() const { return proof_.header(); }  
  proof_type const & proof() const { return proof_; }
  proof_type & proof() { return proof_; }  
  body_type const & body() const { return body_; }
  
private:
  meta_type meta_data_;
  body_type body_;
  proof_type proof_;    
};



template< typename T, typename B, typename P, typename H, typename M >
void Serialize( T & serializer, BasicBlock< B, P, H, M > const &b) {
  serializer <<  b.body() << b.proof() << b.meta_data();
}

template< typename T, typename B, typename P, typename H, typename M  >
void Deserialize( T & serializer, BasicBlock< B, P, H, M > &b) {
  B body;  
  serializer >> body >> b.proof() >> b.meta_data();
  b.SetBody(body);
  
}

};
};
#endif
