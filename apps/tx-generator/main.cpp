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

#include "core/byte_array/byte_array.hpp"
#include "core/commandline/params.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/serializers/counter.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_builder.hpp"
#include "ledger/chain/transaction_serializer.hpp"
#include "storage/resource_mapper.hpp"
#include "vectorise/threading/pool.hpp"

#include <chrono>
#include <fstream>
#include <memory>
#include <random>
#include <vector>

using fetch::commandline::Params;
using fetch::crypto::ECDSASigner;
using fetch::threading::Pool;
using fetch::byte_array::ConstByteArray;
using fetch::ledger::TransactionBuilder;
using fetch::ledger::TransactionSerializer;
using fetch::ledger::Address;
using fetch::serializers::MsgPackSerializer;
using fetch::serializers::SizeCounter;
using fetch::BitVector;
using fetch::storage::ResourceAddress;

using KeyPtr       = std::unique_ptr<ECDSASigner>;
using KeyArray     = std::vector<KeyPtr>;
using AddressArray = std::vector<Address>;
using Clock        = std::chrono::high_resolution_clock;

constexpr std::size_t NUM_TRANSFERS = 1;

int main(int argc, char **argv)
{
  std::size_t count{0};
  std::size_t key_count{0};
  std::string output{};

  // build the parser
  Params parser{};
  parser.add(count, "count", "The number of tx to generate");
  parser.add(key_count, "keys", "The number of tx to generate", std::size_t{100});
  parser.add(output, "output", "The file being generated", std::string{"out.bin"});

  // parse the command line
  parser.Parse(argc, argv);

  KeyArray     keys(key_count);
  AddressArray addresses(key_count);

  // create all the keys
  Pool pool{};

  std::vector<ConstByteArray> encoded_txs(count);

  std::cout << "Generating tx..." << std::endl;
  auto const started = Clock::now();

  // create all the transactions
  std::size_t const num_tx_per_thread = (count + pool.concurrency() - 1) / pool.concurrency();
  for (std::size_t i = 0; i < pool.concurrency(); ++i)
  {
    // compute the range of data to be populated
    std::size_t const start = num_tx_per_thread * i;
    std::size_t const end   = std::min(count, start + num_tx_per_thread);

    pool.Dispatch([start, end, &encoded_txs]() {
      static constexpr std::size_t LOG2_VECTOR_SIZE = 7u;
      static constexpr std::size_t VECTOR_SIZE      = 1u << LOG2_VECTOR_SIZE;

      BitVector vector{VECTOR_SIZE};

      TransactionSerializer serializer{};
      for (std::size_t j = start; j < end; ++j)
      {
        // generate a new key for this transaction
        ECDSASigner from;
        Address     from_address{from.identity()};

        // determine the resource
        ResourceAddress const resource{"fetch.token.state." + from_address.display()};

        // generate the correct shard mask
        vector.SetAllZero();
        vector.set(resource.lane(LOG2_VECTOR_SIZE), 1);  // set the lane for the tx

        // form the transaction
        TransactionBuilder builder{};
        builder.ValidUntil(1000000u);
        builder.ChargeLimit(NUM_TRANSFERS * 5u);
        builder.ChargeRate(1u);
        builder.TargetChainCode("fetch.token", vector);
        builder.Action("wealth");
        builder.Data(R"({"amount": 1000})");
        builder.Signer(from.identity());
        builder.From(from_address);

        // finalise the transaction
        auto tx = builder.Seal().Sign(from).Build();

        // serialize the transaction
        serializer.Serialize(*tx);
        encoded_txs[j] = serializer.data();
      }
    });
  }

  pool.Wait();
  auto const stopped = Clock::now();

  auto const delta_ns = (stopped - started).count();

  auto const tx_rate =
      (static_cast<double>(encoded_txs.size()) * 1e9) / static_cast<double>(delta_ns);
  std::cout << "Generating tx...complete (tx rate: " << tx_rate << ")" << std::endl;

  std::cout << "Generating contents..." << std::endl;

  // determine the size
  SizeCounter counter{};
  counter << encoded_txs;

  std::cout << "Serial size: " << counter.size() << std::endl;

  MsgPackSerializer buffer{};
  buffer.Reserve(counter.size());  // pre-allocate
  buffer << encoded_txs;

  // flush the stream
  std::ofstream output_stream{output.c_str(), std::ios::out | std::ios::binary};
  output_stream.write(buffer.data().char_pointer(),
                      static_cast<std::streamsize>(buffer.data().size()));
  output_stream.close();

  std::cout << "Generating contents...complete" << std::endl;

  return EXIT_SUCCESS;
}
