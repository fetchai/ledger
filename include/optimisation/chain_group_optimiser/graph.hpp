#ifndef OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#define OPTIMISATION_CHAIN_GROUP_OPTIMISER_HPP
#include".hpp"
#include"byte_array/referenced_byte_array.hpp"
#include"memory/rectangular_array.hpp"
#include<unordered_map>
namespace fetch {
namespace optimisers {

struct PuzzleBrick {
  std::vector< uint32_t > groups;
  uint32_t block_time;
  uint32_t block;
};
  
  struct LaneNode {
    std::vector< PuzzleBrick > bricks_;
    std::size_t i;
  };
  
class LaneGroupGraph : public RectangularArray< PuzzleBrick> {
public:
  uint64_t AddBlock(byte_array_type const &hash, std::vector< uint32_t > groups) {
    name_to_id_[hash] = counter_;
    id_to_name_[counter_] = hash;
    ++counter_;
  }

private:
  std::unordered_map<byte_array_type, uint64_t, hasher_type>  name_to_id_;
  std::unordered_map<uint64_t, byte_array_type>  id_to_name_;
  uint64_t counter_ = 0;
  
};
  
};
};

#endif
