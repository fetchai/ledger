#pragma once
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
#include "core/random/lfg.hpp"
#include "core/serializers/counter.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "network/service/types.hpp"
#include <random>

namespace fetch {
namespace common {

uint32_t GetRandom()
{
  static std::random_device                      rd;
  static std::mt19937                            gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

  return dis(gen);
}

byte_array::ConstByteArray GetRandomByteArray(std::size_t length)
{
  // convert to byte array
  byte_array::ByteArray data;
  data.Resize(length);

  for (std::size_t i = 0; i < length; ++i)
  {
    data[i] = static_cast<uint8_t>(GetRandom());
  }

  return {data};
}

// Time related functionality
using time_point = std::chrono::high_resolution_clock::time_point;

time_point TimePoint()
{
  return std::chrono::high_resolution_clock::now();
}

double TimeDifference(time_point t1, time_point t2)
{
  // If t1 before t2
  if (t1 < t2)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  }
  return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t2).count();
}

class NoCopyClass
{
public:
  NoCopyClass()
  {}

  NoCopyClass(int val)
    : class_value{val}
  {}

  NoCopyClass(NoCopyClass &rhs) = delete;
  NoCopyClass &operator=(NoCopyClass &rhs) = delete;
  NoCopyClass(NoCopyClass &&rhs)           = delete;
  NoCopyClass &operator=(NoCopyClass &&rhs) = delete;

  int class_value = 0;
};

template <typename T>
inline void Serialize(T &serializer, NoCopyClass const &b)
{
  serializer << b.class_value;
}

template <typename T>
inline void Deserialize(T &serializer, NoCopyClass &b)
{
  serializer >> b.class_value;
}

template <typename T>
void MakeString(T &str, std::size_t N = 4)
{
  static fetch::random::LaggedFibonacciGenerator<> lfg;
  byte_array::ByteArray                            entry;
  entry.Resize(N);

  for (std::size_t j = 0; j < N; ++j)
  {
    entry[j] = uint8_t(lfg() & 0xFF);
  }
  str = entry;
}

template <typename T>
std::size_t Size(const T &item)
{
  serializers::SizeCounter<service::serializer_type> counter;
  counter << item;
  return counter.size();
}

template <typename T>
T NextTransaction(std::size_t bytesToAdd = 0)
{
  fetch::ledger::MutableTransaction trans;

  trans.PushResource(GetRandomByteArray(64));

  ledger::Signatories signatures;
  signatures[fetch::crypto::Identity{"identity_params", "identity"}] =
      fetch::ledger::Signature{"sig_data", "sig_type"};
  byte_array::ByteArray contract_name, data;
  MakeString(contract_name);
  MakeString(data, 1 + bytesToAdd);

  trans.set_signatures(signatures);
  trans.set_contract_name(std::string{contract_name.char_pointer(), contract_name.size()});
  trans.set_data(data);

  return T::Create(trans);
}

std::size_t Hash(fetch::byte_array::ConstByteArray const &arr)
{
  std::size_t hash = 2166136261;
  for (std::size_t i = 0; i < arr.size(); ++i)
  {
    hash = (hash * 16777619) ^ arr[i];
  }
  return hash;
}

void BlockUntilTime(uint64_t startTime)
{
  // get time as epoch, wait until that time to start
  std::time_t t          = static_cast<std::time_t>(startTime);
  std::tm *   timeout_tm = std::localtime(&t);

  time_t                                timeout_time_t = mktime(timeout_tm);
  std::chrono::system_clock::time_point timeout_tp =
      std::chrono::system_clock::from_time_t(timeout_time_t);

  std::this_thread::sleep_until(timeout_tp);
}

}  // namespace common

namespace network_benchmark {

// Transactions are packaged up into blocks and referred to using a hash
using transaction_type = fetch::ledger::Transaction;
using block_hash       = std::size_t;
using block_type       = std::vector<transaction_type>;
using network_block    = std::pair<block_hash, block_type>;

}  // namespace network_benchmark

}  // namespace fetch
