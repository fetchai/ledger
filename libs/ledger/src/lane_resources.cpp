#include "core/assert.hpp"
#include "ledger/lane_resources.hpp"

namespace fetch {
namespace ledger {

LaneResources::document_type LaneResources::GetOrCreate(resource_id_type const &rid) {
  uint64_t const lane_index = rid.lane(log2_num_lanes_);
  detailed_assert(lane_index < resources_.size());

  return resources_[lane_index]->GetOrCreate(rid);
}

LaneResources::document_type LaneResources::Get(resource_id_type const &rid) {
  uint64_t const lane_index = rid.lane(log2_num_lanes_);
  detailed_assert(lane_index < resources_.size());

  return resources_[lane_index]->Get(rid);
}

void LaneResources::Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) {
  uint64_t const lane_index = rid.lane(log2_num_lanes_);
  detailed_assert(lane_index < resources_.size());

  return resources_[lane_index]->Set(rid, value);
}

LaneResources::bookmark_type LaneResources::Commit(bookmark_type const& b) {
  TODO_FAIL("Interface presumably might need updating?");
  return 0;
}

void LaneResources::Revert(bookmark_type const &b) {
  TODO_FAIL("Interface presumably might need updating?");
}

void LaneResources::Lock(resource_id_type const &rid) {
  uint64_t const lane_index = rid.lane(log2_num_lanes_);
  detailed_assert(lane_index < resources_.size());

  return resources_[lane_index]->Lock(rid);
}

void LaneResources::Unlock(resource_id_type const &rid) {
  uint64_t const lane_index = rid.lane(log2_num_lanes_);
  detailed_assert(lane_index < resources_.size());

  return resources_[lane_index]->Unlock(rid);
}

void LaneResources::HasLock(resource_id_type const &rid) {
  uint64_t const lane_index = rid.lane(log2_num_lanes_);
  detailed_assert(lane_index < resources_.size());

  return resources_[lane_index]->HasLock(rid);
}

StateDatabaseInterface::hash_type LaneResources::Hash() {
  TODO_FAIL("Interface presumably might need updating?");
  return 0;
}

} // namespace ledger
} // namespace fetch
