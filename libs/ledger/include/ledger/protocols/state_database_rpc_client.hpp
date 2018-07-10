#ifndef FETCH_STATE_DATABASE_CLIENT_HPP
#define FETCH_STATE_DATABASE_CLIENT_HPP

#include "network/service/client.hpp"
#include "ledger/state_database_interface.hpp"

namespace fetch {
namespace ledger {

class StateDatabaseRpcClient : public StateDatabaseInterface {
public:

  StateDatabaseRpcClient(byte_array::ConstByteArray const &host, uint16_t const &port,
                         network::ThreadManager const &thread_manager);

/// @name State Database Interface
  /// @{
  document_type GetOrCreate(resource_id_type const &rid) override;
  document_type Get(resource_id_type const &rid) override;
  void Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) override;
  bookmark_type Commit(bookmark_type const& b) override;
  void Revert(bookmark_type const &b) override;
  /// @}
private:

  std::unique_ptr<fetch::service::ServiceClient > service_;

};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_CLIENT_HPP
