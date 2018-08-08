#include "ledger/chaincode/dummy_contract.hpp"

#include <chrono>
#include <random>

static constexpr std::size_t MINIMUM_TIME = 50;
static constexpr std::size_t MAXIMUM_TIME = 200;
static constexpr std::size_t DELTA_TIME   = MAXIMUM_TIME - MINIMUM_TIME;

namespace fetch {
namespace ledger {

DummyContract::DummyContract() : Contract("fetch.dummy")
{
  OnTransaction("wait", this, &DummyContract::Wait);
  OnTransaction("run", this, &DummyContract::Run);
}

DummyContract::Status DummyContract::Wait(transaction_type const &)
{
  std::random_device rd;
  std::mt19937       rng;
  rng.seed(rd());

  // generate a random amount of 'work' time
  std::size_t const work_time = (rng() % DELTA_TIME) + MINIMUM_TIME;

  auto counter = counter_.load();

  fetch::logger.Info("Running complicated transaction ", counter, " ... ");

  // wait for the work time
  std::this_thread::sleep_for(std::chrono::milliseconds(work_time));

  fetch::logger.Info("Running complicated transaction ", counter, " ... complete");

  ++counter_;

  return Status::OK;
}

DummyContract::Status DummyContract::Run(transaction_type const &)
{
  fetch::logger.Info("Running that contract...");
  return Status::OK;
}

}  // namespace ledger
}  // namespace fetch
