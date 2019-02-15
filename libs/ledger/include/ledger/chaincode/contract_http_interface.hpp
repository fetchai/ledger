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

#include "http/module.hpp"
#include "ledger/chaincode/cache.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace fetch {
namespace ledger {

class StorageInterface;
class TransactionProcessor;

class ContractHttpInterface : public http::HTTPModule
{

public:
  static constexpr char const *LOGGING_NAME = "ContractHttpInterface";

  ContractHttpInterface(StorageInterface &storage, TransactionProcessor &processor);

private:
  http::HTTPResponse OnQuery(byte_array::ConstByteArray const &contract_name,
                             byte_array::ConstByteArray const &query,
                             http::HTTPRequest const &         request);

  http::HTTPResponse OnTransaction(
      http::ViewParameters const &, http::HTTPRequest const &request,
      byte_array::ConstByteArray const *const expected_contract_name = nullptr);

  /** @brief Structure containing status of of multi-transaction submission.
   *
   *  The structure is supposed to carry information about how many transactions
   *  have been successfully processed and how many transactions have been
   *  actually received for processing.
   *  The structure is supposed to be used as return value for methods which
   *  are dedicated to handle HTTP request for bulk transaction (multi-transaction)
   *  reception, giving caller ability to check status of how request has been
   *  handled (transaction reception/processing).
   *
   * @see SubmitJsonTx
   * @see SubmitNativeTx
   */
  struct SubmitTxStatus
  {
    std::size_t processed;
    std::size_t received;
  };

  /**
   * Method handles incoming http request containing single or bulk of JSON formatted
   * Wire transactions.
   *
   * @param request https request containing single or bulk of JSON formatted Wire Transaction(s).
   * @param expected_contract_name contract name each transaction in request must conform to.
   *        Transactions which do NOT conform to this contract name will NOT be accepted further
   *        processing. If the value is `nullptr` (default value) the contract name check is
   *        DISABLED, and so transactions (each received transaction in bulk) can have any contract
   *        name.
   *
   * @return submit status, please see the `SubmitTxStatus` structure
   * @see SubmitTxStatus

   * @see SubmitNativeTx
   */
  SubmitTxStatus SubmitJsonTx(
      http::HTTPRequest const &               request,
      byte_array::ConstByteArray const *const expected_contract_name = nullptr);

  /**
   * Method handles incoming http request containing single or bulk of Native formattd
   * Wire transactions.
   *
   * This method was originally designed for benchmark/stress-test purposes, but can be used in
   * production environment.
   *
   * @param request https request containing single or bulk of JSON formatted Wire Transaction(s).
   * @param expected_contract_name contract name each transaction in request must conform to.
   *        Transactions which do NOT conform to this contract name will NOT be accepted further
   *        processing. If the value is `nullptr` (default value) the contract name check is
   *        DISABLED, and so transactions (each received transaction in bulk) can have any contract
   *        name.
   *
   * @return submit status, please see the `SubmitTxStatus` structure
   * @see SubmitTxStatus

   * @see SubmitJsonTx
   */
  SubmitTxStatus SubmitNativeTx(
      http::HTTPRequest const &               request,
      byte_array::ConstByteArray const *const expected_contract_name = nullptr);

  StorageInterface &    storage_;
  TransactionProcessor &processor_;
  ChainCodeCache        contract_cache_;
};

}  // namespace ledger
}  // namespace fetch
