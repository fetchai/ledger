//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "metrics/metric_file_handler.hpp"
#include "core/byte_array/encoders.hpp"

#include <fstream>
#include <mutex>
#include <utility>

using fetch::byte_array::ToBase64;

namespace fetch {
namespace metrics {
namespace {

/**
 * Convert a instrument type to string
 *
 * @param instrument The instrument to convert
 * @return The string representation of the instrument
 */
char const *ToString(MetricHandler::Instrument instrument)
{
  switch (instrument)
  {
  case MetricHandler::Instrument::TRANSACTION:
    return "transaction";

  case MetricHandler::Instrument::BLOCK:
    return "block";
  }

  return "unknown";
}

/**
 * Convert a event type to string
 *
 * @param event The input event type
 * @return The string representation fo the event
 */
char const *ToString(MetricHandler::Event event)
{
  switch (event)
  {
  case MetricHandler::Event::SUBMITTED:
    return "submitted";
  case MetricHandler::Event::STORED:
    return "stored";
  case MetricHandler::Event::SYNCED:
    return "synced";
  case MetricHandler::Event::RECEIVED_FOR_SYNC:
    return "received_for_sync";
  case MetricHandler::Event::QUEUED:
    return "queued";
  case MetricHandler::Event::PACKED:
    return "packed";
  case MetricHandler::Event::EXECUTION_STARTED:
    return "exec-started";
  case MetricHandler::Event::EXECUTION_COMPLETE:
    return "exec-complete";
  case MetricHandler::Event::GENERATED:
    return "generated";
  case MetricHandler::Event::RECEIVED:
    return "received";
  }

  return "unknown";
}

}  // namespace

/**
 * Create a metric file handler with specified filename
 *
 * @param filename The filename of the file to be generated
 */
MetricFileHandler::MetricFileHandler(std::string filename)
  : filename_(std::move(filename))
  , stack_{}
  , active_{true}
  , worker_{&MetricFileHandler::ThreadEntryPoint, this}
{
  FETCH_LOCK(stack_lock_);
  stack_.reserve(BUFFER_SIZE);
}

/**
 * Destructor
 */
MetricFileHandler::~MetricFileHandler()
{
  active_ = false;

  {
    FETCH_LOCK(stack_lock_);
    stack_notify_.notify_all();
  }

  worker_.join();
  worker_ = std::thread{};
}

/**
 * Record a specified metric
 *
 * @param identifier The identifier of the metric
 * @param instrument  The instrument being measured
 * @param event The event being recorded
 * @param timestamp The timestamp of the event
 */
void MetricFileHandler::RecordMetric(ConstByteArray const &identifier, Instrument instrument,
                                     Event event, Timestamp const &timestamp)
{
  // add the entry to the stack
  FETCH_LOCK(stack_lock_);
  stack_.emplace_back(Entry{identifier, instrument, event, timestamp});
  stack_notify_.notify_all();
}

/**
 * Background thread process
 */
void MetricFileHandler::ThreadEntryPoint()
{
  // create the output file stream
  std::ofstream output_file(filename_.c_str());

  // write out the CSV header
  output_file << "Timestamp,Instrument,Event,Identifier" << std::endl;

  // main processing loop
  Entry current;
  while (active_)
  {
    // extract an entry from the stack or wait for one to be available
    {
      std::unique_lock<std::mutex> lock(stack_lock_);

      // if the stack is empty
      if (stack_.empty())
      {
        stack_notify_.wait(lock);
        continue;
      }
      else
      {
        // must have more than one entry
        current = stack_.back();
        stack_.pop_back();
      }
    }
    // generate the CSV entry
    output_file << current.timestamp.time_since_epoch().count() << ','
                << ToString(current.instrument) << ',' << ToString(current.event) << ','
                << ToBase64(current.identifier) << std::endl;
  }
}

}  // namespace metrics
}  // namespace fetch
