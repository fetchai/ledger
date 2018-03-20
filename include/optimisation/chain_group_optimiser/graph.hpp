#ifndef OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#define OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#include"assert.hpp"
#include"crypto/fnv.hpp"
#include"byte_array/const_byte_array.hpp"
#include"memory/rectangular_array.hpp"
#include"commandline/vt100.hpp"
#include<unordered_map>
#include<map>
namespace fetch {
namespace optimisers {

struct PuzzleBrick {
  std::vector< uint32_t > previous;  
  std::unordered_set< uint32_t > groups;
  uint64_t block = uint64_t(-1);
  double work = 0;
  bool in_use = false;
};
  
  
class GroupGraph : public memory::RectangularArray< uint64_t > {
public:
  typedef crypto::CallableFNV hasher_type;
  typedef byte_array::ConstByteArray byte_array_type;

  enum {
    EMPTY = uint64_t(-1)    
  };
  
  GroupGraph(std::size_t const &blocks, std::size_t const &groups):
    memory::RectangularArray< uint64_t >(blocks, groups )
  {
    for(auto &a: *this) a = EMPTY;    
    bricks_.resize(blocks);
  }
    
  uint64_t AddBlock(uint64_t const &block_time,
    byte_array_type const &hash, double work, std::unordered_set< uint32_t > groups) {
    
    if(block_time < block_offset_) {
      TODO_FAIL("Block time is too small");    
    }

    if(name_to_id_.find(hash) != name_to_id_.end() ) {
      TODO_FAIL("Hash already exist");      
    }
    
    name_to_id_[hash] = counter_;
    id_to_name_[counter_] = hash;
    uint64_t id = counter_;
    ++counter_;

    PuzzleBrick brick;
    brick.groups = groups;
    brick.block = id;    

    bricks_[block_time].push_back( brick );    

    return id;
  }

  void Shift() 
  {
    // TODO: shift
    ++block_offset_;    
  }


  std::vector< PuzzleBrick > & bricks(std::size_t const&i) 
  {
    return bricks_[i];
  }

  std::vector< PuzzleBrick > const & bricks(std::size_t const&i) const
  {
    return bricks_[i];
  }
  
  
  byte_array_type name_from_id(uint64_t const& i) const
  {
    return id_to_name_.find(i)->second;
  }

  uint64_t id_from_name(byte_array_type const& name) const
  {
    return name_to_id_.find(name)->second;
  }

  bool Activate(uint64_t const &block, uint64_t const &brick) 
  {
    auto &bricks = bricks_[block];
    auto &b =     bricks[brick];

    bool ret = true;
    for(auto &g: b.groups) {
      ret &= (this->At(block, g)==EMPTY);      
    }    

    if(ret) {

      for(auto &g: b.groups) {
        this->Set(block, g, b.block);        
      }
      b.in_use = true;
      total_work_ += b.work;      
    }
    
    return ret;    
  }

  bool Deactivate(uint64_t const &block, uint64_t const &brick) 
  {
    auto &bricks = bricks_[block];
    auto &b =     bricks[brick];

    bool ret = true;
    for(auto &g: b.groups) {
      ret &= (this->At(block, g)==b.block);      
    }    

    if(ret) {

      for(auto &g: b.groups) {
        this->Set(block, g, EMPTY);        
      }
      b.in_use = false;
      total_work_ -= b.work;
      
    }

    return ret;    
  }
  
private:
  std::vector< std::vector< PuzzleBrick > > bricks_;
  std::unordered_map<byte_array_type, uint64_t, hasher_type>  name_to_id_;
  std::unordered_map<uint64_t, byte_array_type>  id_to_name_;
  uint64_t counter_ = 0;
  uint64_t block_offset_ = 0;
  double total_work_ = 0;
  
};


std::ostream& operator<< (std::ostream& stream, GroupGraph const &graph )
{
  std::size_t lane_width = 3;
  std::size_t lane_width_half = lane_width >> 1;  
  std::size_t ww = graph.width() ;    
  std::size_t w = ww>> 1;
  std::size_t lane_size = lane_width * ww;
  auto DrawLane = [lane_width, ww, w, lane_size, lane_width_half, graph](std::vector< PuzzleBrick > const &bricks, std::size_t &n) { 
    std::size_t start = ww;
    std::size_t end = 0;

    uint64_t block = uint64_t(-1);
        
    PuzzleBrick brick;

    while( (n < bricks.size()) && (!bricks[n].in_use) ) ++n;    
      
    if( (n < bricks.size()) && (bricks[n].in_use) ) {
      brick = bricks[n];     
      for(auto const &g: brick.groups) {
        if(g < start) start = g;
        if(g > end) end = g;          
      }
      block = brick.block;
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
        std::cout << (( (brick.groups.find( i ) != brick.groups.end())  ) ? "*" :"-");
      } else {
        std::cout << "|";        
      }
      
      
      
      for(std::size_t j=0; j < lane_width_half ; ++j)
        std::cout << ( (right) ? "-" : " " );                      
    }

    if(block != uint64_t(-1)) {
      std::cout << ": " << graph.name_from_id(block);
    }
    ++n;
    
  };


  for(std::size_t i=0; i < graph.height() ; ++i)
  {
    auto const &bricks = graph.bricks(i);
    
    std::cout << bricks.size() << " blocks" << std::endl;    
    assert(bricks.size() <= ww);
    std::size_t n = 0;
    
    for(std::size_t j=0; j < w ; ++j)
    {
      std::cout << std::setw(2) << " " << " ";
      DrawLane(bricks, n);        
      std::cout << std::endl;
    }
    std::cout << std::setw(2) << i << " ";
    DrawLane(bricks, n);     
    std::cout << std::endl;
    for(std::size_t j=0; j < w ; ++j)
    {
      std::cout << std::setw(2) << " " << " ";

      DrawLane(bricks, n);      
      
      std::cout << std::endl;
      
    }
    for(std::size_t j=0; j < lane_size +3 ; ++j)
      std::cout << "-";
    std::cout << std::endl;
    
  }
  return stream;  
}  

};
};

#endif
