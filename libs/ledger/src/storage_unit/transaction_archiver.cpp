//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "chain/transaction.hpp"
#include "ledger/storage_unit/transaction_archiver.hpp"
#include "ledger/storage_unit/transaction_pool_interface.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"

#include <chrono>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "TxArchiver";

}  // namespace

TransactionArchiver::TransactionArchiver(uint32_t lane, TransactionPoolInterface &pool,
                                         TransactionStoreInterface &archive)
  : lane_{lane}
  , pool_{pool}
  , archive_{archive}
  , state_machine_{std::make_shared<StateMachine>(LOGGING_NAME, State::COLLECTING, ToString)}
  // clang-format off
  , confirmed_total_{CreateCounter("ledger_txarchiver_confirmed_total", "The total number of transactions added to the confirmed queue")}
  , duplicate_total_{CreateCounter("ledger_txarchiver_duplicate_total", "The total number of transactions duplicate transactions processed")}
  , additions_total_{CreateCounter("ledger_txarchiver_additions_total", "The total number of transactions archived by the archiver")}
  , lost_total_{CreateCounter("ledger_txarchiver_lost_total", "The total number of transactions lost by the archiver")}
  , processed_total_{CreateCounter("ledger_txarchiver_processed_total", "The total number of transactions processed by the archiver")}
// clang-format on
{
  // make the reservation
  digests_.reserve(BATCH_SIZE);

  // configure the state machine
  state_machine_->RegisterHandler(State::COLLECTING, this, &TransactionArchiver::OnCollecting);
  state_machine_->RegisterHandler(State::FLUSHING, this, &TransactionArchiver::OnFlushing);
}

void TransactionArchiver::Confirm(Digest const &digest)
{
  confirmation_queue_.Push(digest);
  confirmed_total_->increment();
}

TransactionArchiver::StateMachinePtr const &TransactionArchiver::GetStateMachine() const
{
  return state_machine_;
}

TransactionArchiver::State TransactionArchiver::OnCollecting()
{
  for (;;)
  {
    // attempt to extract an element in the confirmation queue
    digests_.emplace_back(Digest{});
    bool const extracted = confirmation_queue_.Pop(digests_.back(), milliseconds::zero());

    // update the index if needed
    if (!extracted)
    {
      digests_.pop_back();
    }

    bool const is_buffer_full    = (digests_.size() == BATCH_SIZE);
    bool const is_batch_complete = (!extracted) && (!digests_.empty());

    if (is_buffer_full || is_batch_complete)
    {
      return State::FLUSHING;
    }

    // Queue is empty and nothing to write - trigger delay and do not change FSM state
    if (!extracted)
    {
      state_machine_->Delay(1s);
      break;
    }
  }

  return State::COLLECTING;
}

TransactionArchiver::State TransactionArchiver::OnFlushing()
{
  // check if we need to transition from this state
  if (digests_.empty())
  {
    return State::COLLECTING;
  }

  // flush a transaction to the archive
  {
    auto const &current = digests_.back();

    chain::Transaction tx{};
    if (archive_.Has(current))
    {
      // no op
      duplicate_total_->increment();
    }
    else if (pool_.Get(current, tx))
    {
      // add the transaction to the store
      archive_.Add(tx);

      // remove the transaction from the pool
      pool_.Remove(current);

      additions_total_->increment();
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup tx: 0x", current.ToHex(), " from cache");

      lost_total_->increment();
    }
  }

  // remove the current element
  digests_.pop_back();

  processed_total_->increment();

  return State::FLUSHING;
}

telemetry::CounterPtr TransactionArchiver::CreateCounter(char const *name,
                                                         char const *description) const
{
  telemetry::Measurement::Labels labels{{"lane", std::to_string(lane_)}};
  return telemetry::Registry::Instance().CreateCounter(name, description, std::move(labels));
}

char const *ToString(TransactionArchiver::State state)
{
  char const *text = "Unknown";

  switch (state)
  {
  case TransactionArchiver::State::COLLECTING:
    text = "Collecting";
    break;
  case TransactionArchiver::State::FLUSHING:
    text = "Flushing";
    break;
  }

  return text;
}

}  // namespace ledger
}  // namespace fetch
