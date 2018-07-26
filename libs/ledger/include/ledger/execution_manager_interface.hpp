#pragma once

#include "ledger/chain/block.hpp"

namespace fetch {
namespace ledger {

class ExecutionManagerInterface
{
public:
  using block_type        = chain::BlockBody;
  using block_digest_type = block_type::digest_type;

  enum class Status
  {
    COMPLETE = 0,  ///< The block has been successfully applied/executed
    SCHEDULED,     ///< The block has been successfully scheduled

    // Errors
    NOT_STARTED,      ///< The executor has not been started
    ALREADY_RUNNING,  ///< The executor is already running another block
    NO_PARENT_BLOCK,  ///< The executor has not processed the
    UNABLE_TO_PLAN    ///< The execution manager is unable to plan execution,
                      ///< typically because resources issues
  };

  // Construction / Destruction
  ExecutionManagerInterface()          = default;
  virtual ~ExecutionManagerInterface() = default;

  /// @name Execution Manager Interface
  /// @{
  virtual Status            Execute(block_type const &block) = 0;
  virtual block_digest_type LastProcessedBlock()             = 0;
  virtual bool              IsActive()                       = 0;
  virtual bool              IsIdle()                         = 0;
  virtual bool              Abort()                          = 0;
  /// @}
};

template <typename T>
void Serialize(T &serializer, ExecutionManagerInterface::Status const &status)
{
  serializer << static_cast<uint8_t>(status);
}

template <typename T>
void Deserialize(T &serializer, ExecutionManagerInterface::Status &status)
{
  uint8_t raw = 0xFF;
  serializer >> raw;
  status = static_cast<ExecutionManagerInterface::Status>(raw);
}

}  // namespace ledger
}  // namespace fetch
