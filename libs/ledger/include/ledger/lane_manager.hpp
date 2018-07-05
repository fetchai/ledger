#ifndef FETCH_LANE_MANAGER_HPP
#define FETCH_LANE_MANAGER_HPP

#include "ledger/lane.hpp"
#include "ledger/slice_input_interface.hpp"

#include <memory>
#include <vector>

namespace fetch {
namespace ledger {

// owning class for the lanes of the instance
class LaneManager {
public:
  using LanePtr = std::shared_ptr<Lane>;
  using LaneList = std::vector<LanePtr>;

private:

  LaneList lanes_;

};

} // namespace ledger
} // namespace fetch

#endif //FETCH_LANE_MANAGER_HPP
