#ifndef OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#define OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#include"assert.hpp"
#include"protocols/chain_keeper/block.hpp"

#include"crypto/fnv.hpp"
#include"byte_array/const_byte_array.hpp"
#include"memory/rectangular_array.hpp"

#include"chain/block.hpp"
#include"chain/consensus/proof_of_work.hpp"

#include"commandline/vt100.hpp"

#include<unordered_map>
#include<map>
#include<memory>

namespace fetch {
namespace protocols {
/*
struct Block {
  typedef std::shared_ptr< Block > shared_block_type;
  
  std::vector< shared_block_type > previous;
  std::vector< shared_block_type > next;
  std::unordered_set< uint32_t > groups; 

  uint64_t block = uint64_t(-1);
  double work = 0;
  double total_work = 0;  
  bool in_use = false;
};
*/  

typedef fetch::chain::BasicBlock< BlockBody, fetch::chain::consensus::ProofOfWork, fetch::crypto::SHA256 > Block;

class GroupGraph : public memory::RectangularArray< uint64_t > {
public:

  typedef std::shared_ptr< Block > shared_block_type;  
  typedef crypto::CallableFNV hasher_type;
  typedef byte_array::ConstByteArray byte_array_type;

  enum {
    EMPTY = uint64_t(-1)    
  };
  
  GroupGraph(std::size_t const &blocks, std::size_t const &groups):
    memory::RectangularArray< uint64_t >(blocks, groups )
  {
    for(auto &a: *this) a = EMPTY;    
//    bricks_.resize(blocks);
    bricks_at_block_.resize(blocks);
    block_number_.resize(groups);
    chains_.resize(groups);
    
    for(auto &b: block_number_) b = 0;    
  }
  
  uint16_t AddHash(byte_array_type const &hash) 
  {
    if(name_to_id_.find(hash) != name_to_id_.end() ) {
      TODO_FAIL("Hash already exist: ", hash);      
    }

    name_to_id_[hash] = counter_;
    id_to_name_[counter_] = hash;
    uint64_t id = counter_;
    ++counter_;
    return id;    
  }
  

  uint64_t AddBlock(shared_block_type const &brick) 
  {    
    bricks_.push_back( brick );
    
    if(brick->previous().size() == 0 ) {
      next_blocks_.insert( brick->id() );
      next_refs_[ brick->id() ] = 1; 
    }

    return brick->id();
  }
  
    
  uint64_t AddBlock(double work, byte_array_type const &hash,
    std::unordered_map< uint32_t,  byte_array_type > const &previous_blocks) {

    for(auto &h: previous_blocks) {
      if(h.second == "genesis") continue;
      
      if(name_to_id_.find( h.second ) == name_to_id_.end() ) {
        TODO_FAIL("previous not found");
      }      
    }
     
    uint16_t id = AddHash( hash );
    
    shared_block_type brick = std::make_shared< Block >( );
    brick->set_id( id );
    brick->set_weight( work );
    

    for(auto const &h: previous_blocks) {
      if(h.second == "genesis") {
        brick->add_previous( h.first, shared_block_type() );
      } else {              
        auto id = name_to_id_[h.second];
        auto ptr = bricks_[id];
        
        brick->add_previous( h.first, ptr );
      }
      
    }
    
    return AddBlock( brick );
  }

  shared_block_type CreateBlock(byte_array_type const &hash,
    std::unordered_set< uint32_t > groups, bool add = true) {

    uint64_t id = AddHash( hash );
    
    shared_block_type brick = std::make_shared< Block >( );
//    brick->groups = groups;
    brick->set_id( id );
    //TODO
        
    for(auto &g: groups) {
      auto &chain = chains_[g];
      if(chain.size() > 0) {
//        brick->previous.push_back( chain.back() );        
      }
    }

    if(add) {
      AddBlock( brick );
    }

    
    return brick;
  }
  

  void Shift() 
  {
    // TODO: shift
    ++block_offset_;    
  }


  std::vector< shared_block_type > & bricks(std::size_t const&i) 
  {
    return bricks_at_block_[i];
  }

  std::vector< shared_block_type > const & bricks(std::size_t const&i) const
  {
    return bricks_at_block_[i];
  }
  
  
  byte_array_type name_from_id(uint64_t const& i) const
  {
    return id_to_name_.find(i)->second;
  }

  uint64_t id_from_name(byte_array_type const& name) const
  {
    return name_to_id_.find(name)->second;
  }

  bool Activate(uint64_t block) 
  {
    if(used_blocks_.find(block) != used_blocks_.end()) {
      return false;      
    }
    uint64_t block_n = 0;
    auto &b = bricks_[block];

    
    for(auto &g: b->groups() ) {
      block_n = std::max( block_n,  block_number_[g] );
    }
    if(block_n >= height() ) return false;
    
    std::unordered_map< uint64_t, int > prev_blocks;    

    for(auto &g: b->groups() ) {
      //      std::cout << "  - From group " << g << std::endl;
        
      auto &chain = chains_[g];
      if(chain.size() != 0) {
        if( prev_blocks.find( chain.back()->id()) == prev_blocks.end() ) {
          prev_blocks[ chain.back()->id() ] = 1;
        } else {
          ++prev_blocks[ chain.back()->id() ];
        }
        
      }
    }

    bool ret = true;    
    for(auto &p: b->previous()) {
      if(!p) continue;
      
      if(prev_blocks.find(p->id()) == prev_blocks.end()) {
        ret = false;
        break;        
      }      
      else
      {
        if(prev_blocks[ p->id() ] == 0) {
          ret = false;
          break;            
        }
          
        --prev_blocks[ p->id() ];
      }
    }      

    
    for(auto &pb: prev_blocks) {
      if(pb.second !=0) {
        //        std::cout << "  - ?? " << pb.second << std::endl;             
        ret = false;
        break;        
      }     
    }

    if(ret) {
      for(auto&g: b->groups()) {
        chains_[g].push_back(b);
        block_number_[g] = block_n + 1;

      }
      //      std::cout << " === Block number: " << block_n << " === " <<  std::endl;
      bricks_at_block_[block_n].push_back(b);
    }
    

    return ret;    
  }

  
  std::unordered_set< uint64_t > const &next_blocks() const {
    return next_blocks_;
  }
private:
  std::vector< std::vector< shared_block_type > > chains_;
  std::vector< uint64_t > block_number_;
  
  std::unordered_set< uint64_t > used_blocks_;
  

  
  std::unordered_set< uint64_t > next_blocks_;
  std::unordered_map< uint64_t, int > next_refs_;  
  std::vector< shared_block_type > bricks_;
  std::vector< std::vector< shared_block_type > > bricks_at_block_;  
  std::unordered_map<byte_array_type, uint64_t, hasher_type>  name_to_id_;
  std::unordered_map<uint64_t, byte_array_type>  id_to_name_;
  uint64_t counter_ = 0;
  uint64_t block_offset_ = 0;
  double total_work_ = 0;
  
};


std::ostream& operator<< (std::ostream& stream, GroupGraph const &graph )
{
  typedef typename GroupGraph::shared_block_type shared_block_type;
  
  std::size_t lane_width = 3;
  std::size_t lane_width_half = lane_width >> 1;  
  std::size_t ww = graph.width() ;    
  std::size_t w = ww>> 1;
  std::size_t lane_size = lane_width * ww;
  auto DrawLane = [lane_width, ww, w, lane_size, lane_width_half, graph](std::vector< shared_block_type > const &bricks, std::size_t &n) {

    std::size_t start = ww;
    std::size_t end = 0;
    
    uint64_t block = uint64_t(-1);
        
    shared_block_type brick;
      
    if( (n < bricks.size()) )  {
      brick = bricks[n];     
      for(auto const &g: brick->groups()) {
        if(g < start) start = g;
        if(g > end) end = g;          
      }
      block = brick->id();
    }
    
    bool has_block = start <= end;
    
    for(std::size_t i=0; i < ww; ++i)
    {
      bool embedded  = has_block && (start <= i)  && (i <= end);      
      bool left  = has_block && (start < i)  && (i <= end);
      bool right = has_block && (start <= i)  && (i < end);
      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout << ( (left) ? "-" : " " );

      if(embedded) {        
        std::cout << (( (brick->groups().find( i ) != brick->groups().end())  ) ? "*" :"-");
      } else {
        std::cout << "|";        
      }
      
      
      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout << ( (right) ? "-" : " " );                      
    }

    if(block != uint64_t(-1)) {
      std::cout << ": " << graph.name_from_id(block) << " >> ";
      for(auto &p : brick->previous() ) {
        if(!p) continue;
        
        std::cout << graph.name_from_id( p->id() ) << ", ";        
      }
      
    }
    ++n;

  };

  std::size_t total_transactions = 0;
  for(std::size_t i=0; i < graph.height() ; ++i)
  {
    auto const &bricks = graph.bricks(i);
    if(bricks.size() == 0) break;

    std::cout << " ";    
    for(std::size_t i=0; i < ww; ++i)
    {      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout <<  "=";

      std::cout << "=";
      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout << "=" ;    
    }
    
    
    std::cout << " ### Block " << i << ", " << bricks.size() << " transactions" << std::endl;
    total_transactions += bricks.size();
    assert(bricks.size() <= ww);
    std::size_t n = 0;
    
    for(std::size_t j=0; j < bricks.size() ; ++j)
    {
      std::cout << " ";
      DrawLane(bricks, n);        
      std::cout << std::endl;
    }

    
  }
  std::cout << "Total transactions = " << total_transactions << std::endl;
  return stream;  
}  

};
};

#endif
