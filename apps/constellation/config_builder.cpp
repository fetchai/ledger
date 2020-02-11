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

#include "chain/address.hpp"
#include "config_builder.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "manifest_builder.hpp"
#include "settings.hpp"
#include "vectorise/platform.hpp"

using namespace fetch::constellation;

namespace fetch {
namespace {

struct GenesisDescriptor
{
  char const *name;
  uint32_t    block_interval;
  uint32_t    aeon_period;
  char const *contents;
};

GenesisDescriptor const HARD_CODED_CONFIGS[] = {{"mainnet", 10000u, 250000u,
                                                 R"(
{
  "version": 4,
  "accounts": [
    {
      "address": "SqwmbGN7inUZcpxjTQsfEUfidqLVU1mbEGrEbUTgAJCysDMCD",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2shd257H4btog6Z7QmeFZwenJAAwVbmMvU7urSY2t2UTR9dkmD",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "Zaipc1ZgGBfx2s6CnepBEvEhMDaJFyrM5hwgoysewcmQN5Ufr",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "VMXxCKYih8ztThM46MzR4DCfL7DerAHZoxc5AmSQG6RRmYmgj",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "WL5uJJ8yUsMQDVA26PUpiUkTp5qp82EkYc4eeQV5C1SXTGqzs",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "sniKsgcUg5eZtoq6oYkuJ7jUjtAgQGiipBuYCiKBXUuffHAq",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2LyKggHp2bXpTKA3DWK9oG1UAGQbsFjrwCewJtokFUZY57fb9o",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2UUJ5zkULeAqn3DbBbNBxFTPJwSypHzNMSSjEJ9uUTpTsReZWn",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2fUQqAVhnnorVKyUGqoaNzWTrJ52v1ucgLqMZ6DrbKCokt4Brz",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "7QyVhwFjBr5ZvwLpjWqQwNPVctxbZAQapmjje76rg4YwHsz9Y",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2rfuEKVMBymyHmMKsV3nGf2ANUSPYMmmiNZjmiEkmbCFSzFsKj",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2spWFozeZ3hA2h2iCMCrqoUkLUtEjr6he4TbtgXJRu6T8uxArA",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "hjUcYW1xLgPPPe6Acn5CdA4yDfpDtJyW1q3NHzEtTcm5enFcp",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2M3qJ88JG6A7G5RNpF69Ahen7bSbtq6ymiKcxy8j9APkqoYyM4",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2BPa4uCe2EtgG2sijn2f4dz1osdiUJkABQ3y6ei8mvhvLWmX6x",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2ovimbsyC9inpjHtvYpgFzW7BPbc5HR2rXTcQB7Cb2drZ6pqmi",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2w1LLPswyszQmbGS8xo3r1NG5RX5x1SSysQkK7Sm91KXJ1RY5z",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "NCBpicdAVKHSHFJppP8hL9fYi2yxkAgpAVYeeS52tjJjLWUp8",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "pUaakK9GrzecbDTEy2EPcoVgfH4yhVoLXtmjpy63ptXYbXvtU",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2KP7BjerxYJTtKtAsyUi2xzDyVTtz8Ynn7E2ANxjBqNv4oHZ3j",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "MN2DbQwYSErjjFz6b6Yqa3nRmH25oR45mP4Lnsb3q1VbtQK1b",
      "stake": 0,
      "deed": {
        "thresholds": {
          "transfer": 3,
          "amend": 3
        },
        "signees": {
          "2a2hCnoXwUwJ8pVxQcrYjLs5d6qUuR7s8QvtwLP6V8eQuZMmBd": 1,
          "2SNYiwVw47bL63t8jAgEHMGSdWRz9CMbEM77kegXjfhPCfwumQ": 1,
          "28TTPbCF5ASxqQoTGoLGDHSGqQiuhdkE5VzgXtUFUdT7tkdcRb": 1,
          "2gKDxrHAwh11kyTnwdCLLsUmR5hkFg6viKtsYLtR79quL3RUwK": 1
        }
      },
      "balance": 1152977575
    }
  ],
  "consensus": {
    "aeonOffset": 100,
    "aeonPeriodicity": 25,
    "cabinetSize": 20,
    "entropyRunahead": 2,
    "minimumStake": 1000,
    "startTime": 1576724400,
    "stakers": [
      {
        "identity": "yLMOAb4dp8wU5QDUUTSoVn239es5BR9lmGFWzekKGAeLB0IXzVjlBRz2sDNIZPSSCxcE87MbIwH8ceHHyZUODg==",
        "amount": 10000000000000
      },
      {
        "identity": "fmmfgXo00jjFRzL3ncRGNTOdhsuiF+8c+VDeb6uPBBaxui+7XD7rz0Dxg8beXa4XC/gMuXYP3yS35s3Jlqpqrg==",
        "amount": 10000000000000
      },
      {
        "identity": "NirytLVjNS5kS6Tjju9fvM8gio7WkdiIq+KTcgQO3QIebJT9BoMDKLbTtyZc1e6xai3bKRqcg9i+baIY8/ugzA==",
        "amount": 10000000000000
      },
      {
        "identity": "OUU+BmxMAlYkel5lUpW9/4ZL704txwImYL4LcY+vlq5ybWNe9qOC+G5N+dmTNJLf77XX5nvywNkVnu1Z8NJ6mw==",
        "amount": 10000000000000
      },
      {
        "identity": "kWhtt0I2OPxclFZ43yaFZhA1/9ZV7v4sf+umNnlFqrWnqtY9ZB3Z9xfQFEw67iCj5uNYVi7RMa+JYQLX0ShtRw==",
        "amount": 10000000000000
      },
      {
        "identity": "2dXm1b4pmtNWLcBhGj8QZEN0IcwVhh6c3RHEMQX4ohvLIoK/WhbCtS3pRa6WlbmW3KLxHfyeUuyFttKAGyElkw==",
        "amount": 10000000000000
      },
      {
        "identity": "0IPTQA8Fjo2l9l836K4oGPYurg94hof8BF/gHpXf8h07MU2CAB05zRAmrYA5WgIPHaliA7eDT5Wkb9G1Q/MVBg==",
        "amount": 10000000000000
      },
      {
        "identity": "eATZQVp972zHe5kZvspHZaEJXg2NrAGmyKgpyP39ywZnKd2FFKXOcHPp9WP+RXGq8+IdS3G0e3Y+GpW/iV7/FA==",
        "amount": 10000000000000
      },
      {
        "identity": "ahWOYGVuZ9pyXjcFWuuJdVFsnQuBS7Yn3Ob5C5F4lsw4Xkih31zvEF8MMFsB5wSQM6q7zsVLBPDzahBh7HFsLw==",
        "amount": 10000000000000
      },
      {
        "identity": "QlYZK78tdkWJsg/h36rGgZUj3gEQNudXTz/kY5l4v9Jpil33jmmWlZoHxt8OnIIIhyakpIIHeeD+qygQ4N6Aug==",
        "amount": 10000000000000
      },
      {
        "identity": "kh3Fdek6YUH7X64ZCQTxIRVG4Z2Cy1S+xsJGzEOW2T+QkIkJ6ozG8uUlxaWJY7zDjoUMfnnhmqqy1dJ3flMaIg==",
        "amount": 10000000000000
      },
      {
        "identity": "egTYQbmUQoc5nkfy90VsKDQlZv4pc3zixlbTU9AI+8VYIQprf1AvSOD6eT1h8q1dtQ7ZLIXLDBw80SYrI5rhKw==",
        "amount": 10000000000000
      },
      {
        "identity": "dHrTHRF4hXcZ6FgWWawtj+U3JdaAUgRtvylBhhQm5hJ8Ip3LRJQ9t+nTNBa9U7KGFjcWNr14X2UHcIgT76uNYg==",
        "amount": 10000000000000
      },
      {
        "identity": "Coqg91yna8D7y9tr156tFNeNfk3pLM7v7KrhHA38YNF9vQCHjiZdB38uELr+gxV3m4i3ch7ndGJRieD9VQplcQ==",
        "amount": 10000000000000
      },
      {
        "identity": "Kh49Z4OC+L9a/ShFRxyi3LV1Td62YiSRc7acXjfB2n/nAPC9w7+DMAlR/teARr7gG63tPtsxezI6H8HMh/Cdxw==",
        "amount": 10000000000000
      },
      {
        "identity": "F3n930Sy+EjzCJKLchYJkUB44vHijMRzBFlL+I0lDw1wTfgBtVKFhMMDMmct+YBXVCTVKxCl8es12EaHXxKT9Q==",
        "amount": 10000000000000
      },
      {
        "identity": "6OGrUjhAveOiP847JRw07qYEfurMaKm4/AkvAEFPWN6hSHciTJO8sst6qN2TZJjdyvoN6Le5wqv4SIn11uqNHQ==",
        "amount": 10000000000000
      },
      {
        "identity": "+L42C/t/8Z8UPZJGr2F5syrHBMTMba5HL/46M9vKl5mbVTWicMmGOu6+ujOcrqLcaKKId8+U6g9vhmbJSabAZg==",
        "amount": 10000000000000
      },
      {
        "identity": "GClYZdeQ3lkzBh8+MmdqfqaTSvUjNQNdtiab32I4o1XW8uolV2asfM3+GrJFU1LADMn8T0fAmv9flmiRWp5Sug==",
        "amount": 10000000000000
      },
      {
        "identity": "6Hna0chegzhjrTIk1I0PekSQSEJwahzhhm/OraX0Wumf4LiCSgoT97+QD9f3hGZfM++RPaQT/dc1oR2TnLe2cg==",
        "amount": 10000000000000
      }
    ]
  }
}
)"}};

}  // namespace

/**
 * Determine the network mode based on the settings configuration
 *
 * @param settings The settings of the system
 * @return The calculated nework mode
 */
Constellation::NetworkMode GetNetworkMode(Settings const &settings)
{
  Constellation::NetworkMode mode{Constellation::NetworkMode::PUBLIC_NETWORK};

  if (settings.standalone.value())
  {
    mode = Constellation::NetworkMode::STANDALONE;
  }
  else if (settings.private_network.value())
  {
    mode = Constellation::NetworkMode::PRIVATE_NETWORK;
  }

  return mode;
}

/**
 * Build the Constellation's configuration based on the settings based in.
 *
 * @param settings The system settings
 * @return The configuration
 */
Constellation::Config BuildConstellationConfig(Settings const &settings)
{
  Constellation::Config cfg;

  BuildManifest(settings, cfg.manifest);
  cfg.log2_num_lanes        = platform::ToLog2(settings.num_lanes.value());
  cfg.num_slices            = settings.num_slices.value();
  cfg.num_executors         = settings.num_executors.value();
  cfg.db_prefix             = settings.db_prefix.value();
  cfg.processor_threads     = settings.num_processor_threads.value();
  cfg.verification_threads  = settings.num_verifier_threads.value();
  cfg.max_peers             = settings.max_peers.value();
  cfg.transient_peers       = settings.transient_peers.value();
  cfg.block_interval_ms     = settings.block_interval.value();
  cfg.aeon_period           = settings.aeon_period.value();
  cfg.max_cabinet_size      = settings.max_cabinet_size.value();
  cfg.stake_delay_period    = settings.stake_delay_period.value();
  cfg.peers_update_cycle_ms = settings.peer_update_interval.value();
  cfg.disable_signing       = settings.disable_signing.value();
  cfg.sign_broadcasts       = false;
  cfg.kademlia_routing      = settings.kademlia_routing.value();
  cfg.proof_of_stake        = settings.proof_of_stake.value();
  cfg.network_mode          = GetNetworkMode(settings);
  cfg.features              = settings.experimental_features.value();
  cfg.enable_agents         = settings.enable_agents.value();
  cfg.messenger_port        = settings.messenger_port.value();

  // load the genesis file if it exists
  std::string const &genesis_file_path = settings.genesis_file_location.value();
  cfg.genesis_file_contents            = core::ReadContentsOfFile(genesis_file_path.c_str());

  // evaluate our hard coded genesis files
  std::string network_name = settings.network_name.value();

  // Setting the network to local if it is a standalone node
  if (settings.standalone.value() && network_name == "local")
  {
    auto mainaccount       = variant::Variant::Object();
    mainaccount["address"] = settings.initial_address.value();
    mainaccount["balance"] = 1152997575;
    mainaccount["stake"]   = 0;

    auto accounts = variant::Variant::Array(1);
    accounts[0]   = mainaccount;

    variant::Variant config = variant::Variant::Object();
    config["version"]       = 4;
    config["accounts"]      = accounts;

    std::stringstream contents{""};
    contents << config;

    cfg.genesis_file_contents = contents.str();
    return cfg;
  }

  if (cfg.genesis_file_contents.empty() && !network_name.empty())
  {
    for (auto const &genesis : HARD_CODED_CONFIGS)
    {
      if (network_name == genesis.name)
      {
        FETCH_LOG_INFO("ConfigBuilder", "Using ", genesis.name, " configuration");

        cfg.genesis_file_contents = genesis.contents;
        cfg.block_interval_ms     = genesis.block_interval;
        cfg.aeon_period           = genesis.aeon_period;
        cfg.proof_of_stake        = true;
        break;
      }
    }
  }

  // if we did not specify a genesis path, and we could not load one from alternative means attempt
  // to load the contents from the "default" filename
  if (genesis_file_path.empty() && cfg.genesis_file_contents.empty())
  {
    cfg.genesis_file_contents = core::ReadContentsOfFile("genesis_file.json");
  }

  return cfg;
}

}  // namespace fetch
