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

#include "chain/transaction_builder.hpp"
#include "chain/transaction_serializer.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/digest.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "vectorise/threading/pool.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using fetch::byte_array::ConstByteArray;
using fetch::crypto::ECDSASigner;
using fetch::chain::TransactionBuilder;
using fetch::chain::TransactionSerializer;
using fetch::chain::Address;
using fetch::threading::Pool;
using fetch::serializers::LargeObjectSerializeHelper;

using SignerPtr  = std::unique_ptr<ECDSASigner>;
using AddressPtr = std::unique_ptr<Address>;

static std::vector<SignerPtr> GenerateSignersInParallel(std::size_t count)
{
  Pool pool{};

  std::vector<SignerPtr> signers(count);

  std::size_t const batch_size = (count + (pool.concurrency() + 1)) / pool.concurrency();
  for (std::size_t batch = 0; batch < pool.concurrency(); ++batch)
  {
    std::size_t const start = batch * batch_size;
    std::size_t const end   = std::min(count, start + batch_size);

    pool.Dispatch([&signers, start, end]() {
      // create this batch of keys
      for (std::size_t i = start; i < end; ++i)
      {
        signers.at(i) = std::make_unique<ECDSASigner>();
      }
    });
  }

  // wait for all the tasks to complete
  pool.Wait();

  return signers;
}

static std::vector<SignerPtr> GenerateSigners(std::size_t count)
{
  std::cout << "Generating Keys..." << std::endl;

  // switch to threaded model if there are too many of them
  if (count > 1000)
  {
    return GenerateSignersInParallel(count);
  }

  std::vector<SignerPtr> output;
  output.reserve(count);

  for (std::size_t i = 0; i < count; ++i)
  {
    output.emplace_back(std::make_unique<ECDSASigner>());
  }

  std::cout << "Generating Keys...complete" << std::endl;

  return output;
}

static std::vector<AddressPtr> GenerateAddresses(std::vector<SignerPtr> const &signers)
{
  std::cout << "Generating Addresses..." << std::endl;

  std::vector<AddressPtr> output;
  output.reserve(signers.size());

  for (auto const &signer : signers)
  {
    output.emplace_back(std::make_unique<Address>(signer->identity()));
  }

  std::cout << "Generating Addresses...complete" << std::endl;

  return output;
}

static std::vector<ConstByteArray> GenerateTransactionsInParallel(
    std::size_t count, std::vector<SignerPtr> const &signers,
    std::vector<AddressPtr> const &addresses)
{
  Pool pool{};

  // size the output array
  std::vector<ConstByteArray> encoded_tx(count);

  std::size_t const num_batches = signers.size() - 1u;
  std::size_t const batch_size  = (count + (num_batches - 1)) / num_batches;

  SignerPtr const &reference_signer = signers.at(0);

  // create copies of the signing keys to avoid locking
  std::vector<SignerPtr> signer_copies(num_batches);
  for (auto &signer : signer_copies)
  {
    signer = std::make_unique<ECDSASigner>(reference_signer->private_key());
  }

  for (std::size_t batch = 0; batch < num_batches; ++batch)
  {
    std::size_t const start = batch * batch_size;
    std::size_t const end   = std::min(count, start + batch_size);

    // make a fresh copy of the
    auto const &key    = signer_copies.at(batch);
    auto const &target = addresses.at(batch + 1);

    pool.Dispatch([&encoded_tx, &target, start, end, &key]() {
      Address from{key->identity()};

      for (std::size_t i = start; i < end; ++i)
      {
        // build the transaction
        auto const tx = TransactionBuilder()
                            .From(from)
                            .ValidUntil(500)
                            .ChargeRate(1)
                            .ChargeLimit(5)
                            .Transfer(*target, 10)
                            .Signer(key->identity())
                            .Counter(i)
                            .Seal()
                            .Sign(*key)
                            .Build();

        // serialise the transaction
        TransactionSerializer serializer{};
        serializer << *tx;

        // store the transaction into the array
        encoded_tx.at(i) = serializer.data();
      }
    });
  }

  // wait for all the tasks to complete
  pool.Wait();

  // sanity check
  for (auto const &element : encoded_tx)
  {
    if (element.empty())
    {
      throw std::runtime_error("Failed to generate a transction");
    }
  }

  return encoded_tx;
}

static std::vector<ConstByteArray> GenerateTransactions(std::size_t                    count,
                                                        std::vector<SignerPtr> const & signers,
                                                        std::vector<AddressPtr> const &addresses)
{
  std::cout << "Generating transactions..." << std::endl;

  if (count > 1000)
  {
    return GenerateTransactionsInParallel(count, signers, addresses);
  }

  std::vector<ConstByteArray> encoded_tx{};
  encoded_tx.reserve(count);

  for (std::size_t i = 0; i < count; ++i)
  {
    std::size_t const target_index =
        (i % (signers.size() - 1u)) + 1u;  // first index is always the source

    // build the transaction
    auto const tx = TransactionBuilder()
                        .From(*addresses.at(0))
                        .ValidUntil(500)
                        .ChargeRate(1)
                        .ChargeLimit(5)
                        .Counter(i)
                        .Transfer(*addresses.at(target_index), 10)
                        .Signer(signers.at(0)->identity())
                        .Seal()
                        .Sign(*signers.at(0))
                        .Build();

    // serialise the transaction
    TransactionSerializer serializer{};
    serializer << *tx;

    encoded_tx.emplace_back(serializer.data());
  }

  std::cout << "Generating transactions...complete" << std::endl;

  return encoded_tx;
}

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << "<count> <filename> <metapath>" << std::endl;
    return 1;
  }

  auto const        count       = static_cast<std::size_t>(atoi(argv[1]));
  std::string const output_path = argv[2];
  std::string const meta_path   = argv[3];
  std::size_t const num_signers =
      (count / 10u) + 2u;  // 10% of count with minimum of 2 signers (src and dest)

  auto const signers    = GenerateSigners(num_signers);
  auto const addresses  = GenerateAddresses(signers);
  auto const encoded_tx = GenerateTransactions(count, signers, addresses);

  std::cout << "Reference Address: " << addresses.at(0)->display() << std::endl;

  std::cout << "Generating bitstream..." << std::endl;
  LargeObjectSerializeHelper helper{};
  helper << encoded_tx;
  std::cout << "Generating bitstream...complete" << std::endl;

  // verify
  std::vector<ConstByteArray> verified{};
  LargeObjectSerializeHelper  helper2{helper.data()};
  helper2 >> verified;

  std::cout << "Count: " << verified.size() << std::endl;

  std::cout << "Writing to disk ..." << std::endl;

  // write out the binary file
  std::ofstream stream(output_path.c_str(), std::ios::out | std::ios::binary);
  stream.write(helper.data().char_pointer(), static_cast<std::streamsize>(helper.data().size()));

  std::cout << "Writing to disk ... complete" << std::endl;

  std::ofstream stream2(meta_path.c_str());
  stream2 << addresses.at(0)->display() << std::endl;

  return 0;
}
