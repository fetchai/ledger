#include "ledger/chaincode/dummy_contract.hpp"

#include <random>
#include <chrono>

static constexpr std::size_t MINIMUM_TIME = 50;
static constexpr std::size_t MAXIMUM_TIME = 200;
static constexpr std::size_t DELTA_TIME = MAXIMUM_TIME - MINIMUM_TIME;

namespace fetch {
namespace ledger {

DummyContract::DummyContract() // TODO: (EJF) Rename this class
  : Contract("fetch.dummy") {
  OnTransaction("wait", this, &DummyContract::Wait);
}

DummyContract::Status DummyContract::Wait(transaction_type const &tx) {
  std::random_device rd;
  std::mt19937 rng;
  rng.seed(rd());

  // generate a random amount of 'work' time
  std::size_t const work_time = (rng() % DELTA_TIME) + MINIMUM_TIME;

//  logger.Info("Executing transaction: ", byte_array::ToBase64(hash), " slot: ", slot, ' ', work_time,
//              "ms");

  // wait for the work time
  std::this_thread::sleep_for(std::chrono::milliseconds(work_time));
  ++counter_;

  return Status::OK;
}

} // namespace ledger
} // namespace fetch
