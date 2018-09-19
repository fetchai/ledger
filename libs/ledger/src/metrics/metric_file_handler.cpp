//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/metrics/metric_file_handler.hpp"
#include "core/byte_array/encoders.hpp"

#include <fstream>

using fetch::byte_array::ToBase64;

namespace fetch {
namespace ledger {
namespace {

char const *ToString(MetricHandler::Instrument instrument)
{
  switch (instrument)
  {
  case MetricHandler::Instrument::TRANSACTION:
    return "transaction";
  default:
    return "unknown";
  }
}

char const *ToString(MetricHandler::Event event)
{
  switch (event)
  {
  case MetricHandler::Event::SUBMITTED:
    return "submitted";
  case MetricHandler::Event::QUEUED:
    return "queued";
  case MetricHandler::Event::PACKED:
    return "packed";
  case MetricHandler::Event::EXECUTION_STARTED:
    return "exec-started";
  case MetricHandler::Event::EXECUTION_COMPLETE:
    return "exec-complete";
  default:
    return "unknown";
  }
}

}  // namespace

MetricFileHandler::MetricFileHandler(std::string filename)
  : filename_(std::move(filename))
  , entry_stack_{}
  , active_{true}
  , worker_{&MetricFileHandler::ThreadEntryPoint, this}
{
  entry_stack_.reserve(BUFFER_SIZE);
}

MetricFileHandler::~MetricFileHandler()
{
  active_ = false;
  entry_stack_notify_.notify_all();
  worker_.join();
  worker_ = std::thread{};
}

void MetricFileHandler::RecordMetric(ConstByteArray const &identifier, Instrument instrument,
                                     Event event, Timestamp const &timestamp)
{
  // add the entry to the stack
  {
    FETCH_LOCK(entry_stack_lock_);
    entry_stack_.emplace_back(Entry{identifier, instrument, event, timestamp});
  }

  // notify / wake up the worker
  entry_stack_notify_.notify_all();
}

void MetricFileHandler::ThreadEntryPoint()
{
  // create the output file stream
  std::ofstream output_file(filename_.c_str());

  // write out the CSV header
  output_file << "Timestamp,Instrument,Event,Identifier" << std::endl;

  // main processing loop
  std::mutex mutex;
  Entry      current;
  while (active_)
  {
    // extract an entry from the stack or wait for one to be available
    {
      std::unique_lock<std::mutex> lock(mutex);

      // if the stack is empty
      if (entry_stack_.empty())
      {
        entry_stack_notify_.wait(lock);
        continue;
      }
      else
      {
        // must have more than one entry
        current = entry_stack_.back();
        entry_stack_.pop_back();
      }
    }
    // generate the CSV entry
    output_file << current.timestamp.time_since_epoch().count() << ','
                << ToString(current.instrument) << ',' << ToString(current.event) << ','
                << ToBase64(current.identifier) << std::endl;
  }
}

}  // namespace ledger
}  // namespace fetch
