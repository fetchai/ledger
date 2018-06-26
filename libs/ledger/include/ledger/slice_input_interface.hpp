#ifndef FETCH_SLICE_INPUT_INTERFACE_HPP
#define FETCH_SLICE_INPUT_INTERFACE_HPP

#include "ledger/contract_execution_interface.hpp"

namespace fetch {
namespace ledger {

class SliceInputInterface {
public:

  using Placeholder = int;
  using BlockSlice = Placeholder;
  using BlockHeader = Placeholder;

  virtual void OnNewBlockSlice(BlockHeader const &header, BlockSlice const &slice) = 0;
  virtual bool AdvanceSlot() = 0;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_SLICE_INPUT_INTERFACE_HPP
