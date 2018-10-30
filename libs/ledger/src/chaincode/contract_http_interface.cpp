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

#include "ledger/chaincode/contract_http_interface.hpp"

namespace fetch {
namespace ledger {
namespace {

struct AdaptedTx
{
  chain::MutableTransaction tx;
  chain::TxSigningAdapter<chain::MutableTransaction> adapter{tx};

  template <typename T>
  friend void Deserialize(T &serializer, AdaptedTx &tx)
  {
    serializer >> tx.adapter;
  }
};

} // namespace

constexpr char const *           ContractHttpInterface::LOGGING_NAME;
byte_array::ConstByteArray const ContractHttpInterface::API_PATH_CONTRACT_PREFIX("/api/contract/");
byte_array::ConstByteArray const ContractHttpInterface::CONTRACT_NAME_SEPARATOR(".");
byte_array::ConstByteArray const ContractHttpInterface::PATH_SEPARATOR("/");


std::size_t ContractHttpInterface::SubmitJsonTx(http::HTTPRequest const &request)
{
  std::size_t submitted{0};

  // parse the JSON request
  json::JSONDocument doc{request.body()};

  if (doc.root().is_array())
  {
    for (std::size_t i = 0, end = doc.root().size(); i < end; ++i)
    {
      auto const &tx_obj = doc[i];

      // assume single transaction
      auto tx = chain::VerifiedTransaction::Create(chain::FromWireTransaction(tx_obj));

      // add the transaction to the processor
      processor_.AddTransaction(tx);
      ++submitted;
    }
  }
  else
  {
    // assume single transaction
    auto tx = chain::VerifiedTransaction::Create(chain::FromWireTransaction(doc.root()));

    // add the transaction to the processor
    processor_.AddTransaction(tx);
    ++submitted;
  }

  return submitted;
}

std::size_t ContractHttpInterface::SubmitNativeTx(http::HTTPRequest const &request)
{
  std::vector<AdaptedTx> transactions;

  serializers::ByteArrayBuffer buffer(request.body());
  buffer >> transactions;

  std::vector<chain::VerifiedTransaction> verified_tx;
  verified_tx.reserve(transactions.size());

  for (auto const &input_tx : transactions)
  {
    // verify the transaction
    verified_tx.emplace_back(chain::VerifiedTransaction::Create(input_tx.tx));
  }

  processor_.AddTransactions(verified_tx);

  return transactions.size();
}

}  // namespace ledger
}  // namespace fetch
