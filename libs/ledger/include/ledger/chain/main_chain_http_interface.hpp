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

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/chain/main_chain.hpp"
#include "miner/resource_mapper.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace ledger {

class MainChainHTTPInterface : public http::HTTPModule
{
public:
  using MainChain = ledger::MainChain;

  static constexpr char const *LOGGING_NAME = "MainChainHTTPInterface";

  MainChainHTTPInterface(uint32_t log2_num_lanes, MainChain &chain)
    : log2_num_lanes_(log2_num_lanes)
    , chain_(chain)
  {
    Get("/api/main-chain/list-blocks",
        [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
          std::cout << "WAS HERE! 222" << std::endl;
          return GetChain(params, request);
        });
  }

private:
  using Variant = variant::Variant;

  http::HTTPResponse GetChain(http::ViewParameters const & /*params*/,
                              http::HTTPRequest const &request)
  {
    std::size_t chain_length         = 20;
    bool        include_transactions = false;

    if (request.query().Has("size"))
    {
      chain_length = static_cast<std::size_t>(request.query()["size"].AsInt());
    }

    if (request.query().Has("tx"))
    {
      include_transactions = true;
    }

    Variant response = GenerateBlockList(include_transactions, chain_length);

    return http::CreateJsonResponse(response);
  }

  Variant GenerateBlockList(bool include_transactions, std::size_t length)
  {
    using byte_array::ToBase64;

    // lookup the blocks from the heaviest chain
    std::cout << "GETTING BLOCKS: " << length << std::endl;
    auto blocks = chain_.GetHeaviestChain(length);

    Variant block_list = Variant::Array(blocks.size());

    // loop through and generate the complete block list
    std::size_t block_idx{0};
    for (auto &b : blocks)
    {
      // format the block number
      auto block = Variant::Object();

      block["hash"]         = byte_array::ToBase64(b->body.hash);
      block["previousHash"] = byte_array::ToBase64(b->body.previous_hash);
      block["merkleHash"]   = byte_array::ToBase64(b->body.merkle_hash);
      block["proof"]        = byte_array::ToBase64(b->proof.header());
      block["miner"]        = byte_array::ToBase64(b->body.miner);
      block["blockNumber"]  = b->body.block_number;

      // TODO(private issue 532): Remove legacy API
      block["currentHash"] = byte_array::ToBase64(b->body.hash);

      if (include_transactions)
      {
        auto const &slices = b->body.slices;

        Variant slice_list = Variant::Array(slices.size());

        std::size_t slice_idx{0};
        for (auto const &slice : slices)
        {
          Variant transaction_list = Variant::Array(slice.size());

          std::size_t tx_idx{0};
          for (auto const &transaction : slice)
          {
            Variant tx_obj         = Variant::Object();
            tx_obj["digest"]       = ToBase64(transaction.transaction_hash);
            tx_obj["fee"]          = transaction.fee;
            tx_obj["contractName"] = transaction.contract_name;

            Variant resources_array = Variant::Array(transaction.resources.size());

            std::size_t res_idx{0};
            for (auto const &resource : transaction.resources)
            {
              Variant res_obj     = Variant::Object();
              res_obj["resource"] = ToBase64(resource);
              res_obj["lane"] =
                  miner::MapResourceToLane(resource, transaction.contract_name, log2_num_lanes_);

              resources_array[res_idx++] = res_obj;
            }

            tx_obj["resources"]        = resources_array;
            transaction_list[tx_idx++] = tx_obj;
          }

          slice_list[slice_idx++] = transaction_list;
        }

        block["slices"] = slice_list;
      }

      // store the block in the array
      block_list[block_idx++] = block;
    }

    return block_list;
  }

  uint32_t   log2_num_lanes_;
  MainChain &chain_;
};

}  // namespace ledger
}  // namespace fetch