#ifndef FETCH_STATE_DATABASE_PROTOCOL_HPP
#define FETCH_STATE_DATABASE_PROTOCOL_HPP

#include "network/service/protocol.hpp"
#include "ledger/state_database_interface.hpp"


namespace fetch {
namespace ledger {

class StateDatabaseRpcProtocol : public fetch::service::Protocol {
public:
  enum {
    RPC_ID_GET = 1,
    RPC_ID_SET,
    RPC_ID_COMMIT,
    RPC_ID_REVERT
  };

  explicit StateDatabaseRpcProtocol(StateDatabaseInterface &db)
    : database_{db} {

    // expose the interface
    Expose(RPC_ID_GET, &database_, &StateDatabaseInterface::Get);
    Expose(RPC_ID_SET, &database_, &StateDatabaseInterface::Set);
    Expose(RPC_ID_COMMIT, &database_, &StateDatabaseInterface::Commit);
    Expose(RPC_ID_REVERT, &database_, &StateDatabaseInterface::Revert);
  }

private:

  StateDatabaseInterface &database_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_PROTOCOL_HPP
