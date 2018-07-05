#ifndef FETCH_LANE_RESOURCES_HPP
#define FETCH_LANE_RESOURCES_HPP

#include "core/byte_array/referenced_byte_array.hpp"
#include "ledger/state_database_interface.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * Object that encapsulates communicating with all the resources in the system.
 *
 * The helper class will correctly dispatch calls to the required lanes based on
 * the resource id
 */
class LaneResources : public StateDatabaseInterface {
public:
  using state_interface_type = std::shared_ptr<StateDatabaseInterface>;
  using state_interface_list_type = std::vector<state_interface_type>;
  using resource_group_type = resource_id_type::resource_group_type;

  /// @name State Database Interface
  /// @{
  document_type GetOrCreate(resource_id_type const &rid) override;
  document_type Get(resource_id_type const &rid) override;
  void Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) override;

  void Lock(resource_id_type const &rid) override;

  void Unlock(resource_id_type const &rid) override;

  void HasLock(resource_id_type const &rid) override;

  hash_type Hash() override;

  bookmark_type Commit(bookmark_type const& b) override;
  void Revert(bookmark_type const &b) override;
  /// @}

  std::size_t size() const {
    return resources_.size();
  }

  void SetNumLanes(resource_group_type log2_num_lanes) {
    log2_num_lanes_ = log2_num_lanes;
  }

private:

  resource_group_type log2_num_lanes_;
  state_interface_list_type resources_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_LANE_RESOURCES_HPP
