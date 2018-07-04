#ifndef FETCH_STATE_DATABASE_CLIENT_HPP
#define FETCH_STATE_DATABASE_CLIENT_HPP

#include "network/service/client.hpp"
#include "ledger/state_database_interface.hpp"

namespace fetch {
namespace ledger {

class StateDatabaseRpcClient : public service::ServiceClient<network::TCPClient>
                             , public StateDatabaseInterface {
public:

  StateDatabaseRpcClient(byte_array::ConstByteArray const &host, uint16_t const &port,
                         thread_manager_type const &thread_manager);

/// @name State Database Interface
  /// @{
  bool Get(resource_id_type const &rid, byte_array::ByteArray &data) override;
  bool Set(resource_id_type const &rid, byte_array::ConstByteArray const &value) override;
  bookmark_type Commit(bookmark_type const &b) override;
  void Revert(bookmark_type const &b) override;
  /// @}
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_CLIENT_HPP
