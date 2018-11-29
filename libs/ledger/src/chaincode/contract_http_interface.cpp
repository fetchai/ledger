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
  chain::MutableTransaction                          tx;
  chain::TxSigningAdapter<chain::MutableTransaction> adapter{tx};

  template <typename T>
  friend void Deserialize(T &serializer, AdaptedTx &tx)
  {
    serializer >> tx.adapter;
  }
};

}  // namespace

constexpr char const *           ContractHttpInterface::LOGGING_NAME;
byte_array::ConstByteArray const ContractHttpInterface::API_PATH_CONTRACT_PREFIX("/api/contract/");
byte_array::ConstByteArray const ContractHttpInterface::CONTRACT_NAME_SEPARATOR(".");
byte_array::ConstByteArray const ContractHttpInterface::PATH_SEPARATOR("/");

//using THM = ContractHttpInterface::TransactionHandlerMap;
//THM const ContractHttpInterface::transaction_handlers_({
//  THM::value_type{"fetch.token.transfer", &ContractHttpInterface::OnTransfer},
//  THM::value_type{"fetch.token.wealth", &ContractHttpInterface::OnWealth}
//});

ContractHttpInterface::SubmitTxRetval ContractHttpInterface::SubmitJsonTx(http::HTTPRequest const &request, byte_array::ConstByteArray const* const expected_contract_name)
{
  std::size_t submitted{0};
  std::size_t expected_count{0};

  // parse the JSON request
  json::JSONDocument doc{request.body()};

  if (doc.root().IsArray())
  {
    expected_count = doc.root().size();
    for (std::size_t i = 0, end = doc.root().size(); i < end; ++i)
    {
      auto const &tx_obj = doc[i];

      chain::MutableTransaction tx{chain::FromWireTransaction(tx_obj)};

      if (expected_contract_name && tx.contract_name() != *expected_contract_name)
      {
        continue;
      }

      // add the transaction to the processor
      processor_.AddTransaction(std::move(tx));
      ++submitted;
    }
  }
  else
  {
    expected_count = 1;
    chain::MutableTransaction tx{chain::FromWireTransaction(doc.root())};

    if (expected_contract_name && tx.contract_name() == *expected_contract_name)
    {
      // add the transaction to the processor
      processor_.AddTransaction(std::move(tx));
      ++submitted;
    }
  }

  return SubmitTxRetval{submitted, expected_count};
}

ContractHttpInterface::SubmitTxRetval ContractHttpInterface::SubmitNativeTx(http::HTTPRequest const &request, byte_array::ConstByteArray const* const expected_contract_name)
{
  std::vector<AdaptedTx> transactions;

  serializers::ByteArrayBuffer buffer(request.body());
  buffer >> transactions;

  std::size_t submitted{0};
  for (auto const &input_tx : transactions)
  {
    if (expected_contract_name && input_tx.tx.contract_name() != *expected_contract_name)
    {
      continue;
    }

    processor_.AddTransaction(input_tx.tx);
    ++submitted;
  }
  return SubmitTxRetval{submitted, transactions.size()};
}

}  // namespace ledger
}  // namespace fetch
