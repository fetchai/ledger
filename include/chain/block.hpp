#ifndef CHAIN_BLOCK_HPP
#define CHAIN_BLOCK_HPP

#include"serializer/byte_array_buffer.hpp"

#include<memory>

namespace fetch {
namespace chain {
  
template< typename B, typename P, typename H >
class BasicBlock : public std::enable_shared_from_this< BasicBlock< B, P, H > > {
public:
  typedef B body_type;
  typedef P proof_type;
  typedef H hasher_type;
  typedef typename proof_type::header_type header_type;
  typedef BasicBlock< B, P, H > self_type;
  typedef std::shared_ptr< self_type > shared_block_type;
  
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


  void add_previous(uint32_t const &group, shared_block_type const &p) {
    group_to_previous_[group] = p;
    previous_.push_back( p );
    this->body_.groups.push_back(group); // TODO: Ugly hack    
  }

  void add_group(uint32_t const &group ) {
    auto p = shared_block_type();
    add_previous( group, p );

    
  }

  
  shared_block_type previous_from_group(uint32_t const &group) const {
    auto it = group_to_previous_.find(group);
    if(it == group_to_previous_.end()) return shared_block_type();    
    return it->second;
  }

  
  std::vector< shared_block_type > previous() const {
    return previous_;
  }  
  
  shared_block_type shared_block() {
    return this->shared_from_this();
  }

  double const& weight() const { return weight_; } 
  double const& total_weight() const 
  {
    return total_weight_;
  }

  double &weight() { return weight_; } 

  void set_weight(double const& d) 
  {
    weight_ = d;
  }
  
  double &total_weight() 
  {
    return total_weight_;
  }

  void set_total_weight(double const& d) 
  {
    total_weight_ = d;
  }
  
  void set_is_loose(bool b) 
  {
    is_loose_ = b;
  }
  
  bool const &is_loose() const
  {
    return is_loose_;
  }  

  bool const &is_verified() const
  {
    return is_verified_;
  }  

  uint64_t block_number() const   // TODO Move to body
  {
    return block_number_;    
  }
  
  void set_block_number(uint64_t const &b) // TODO: Move to body
  {
    block_number_ = b;    
  }

  uint64_t id() 
  {
    return id_;
  }

  void set_id(uint64_t const &id) 
  {
    id_ = id;    
  }
  /*
  std::unordered_set< uint32_t > groups() {
    return groups_;
  }   
  */
private:

  body_type body_;
  proof_type proof_;

  // META data  and internal book keeping
  uint64_t block_number_;  
  double weight_ = 0 ;
  double total_weight_ = 0;

  // Non-serialized meta-data
//  std::unordered_set< uint32_t > groups_;   
  std::unordered_map< uint32_t, shared_block_type > group_to_previous_; 
  std::vector< shared_block_type > previous_;

  bool is_loose_ = true;
  bool is_verified_ = false;

  uint64_t id_ = uint64_t(-1);
  
  
  
  template< typename AT, typename AB, typename AP, typename AH >
  friend void Serialize( AT & serializer, BasicBlock< AB, AP, AH > const &);
  

  template< typename AT, typename AB, typename AP, typename AH>
  friend void Deserialize( AT & serializer, BasicBlock< AB, AP, AH > &b);  
};


template< typename T, typename B, typename P, typename H >
void Serialize(T& serializer, BasicBlock< B, P, H > const &b) 
{
  serializer <<  b.body() << b.proof() ;  
}

template< typename T, typename B, typename P, typename H >
void Deserialize(T& serializer, BasicBlock< B, P, H > &b) 
{
 
  B body;  
  serializer >> body >> b.proof() ;  
  b.SetBody(body);
  
}

};
};
#endif
