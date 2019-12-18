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

#include "core/filesystem/read_file_contents.hpp"
#include "config_builder.hpp"
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
  char const *contents;
};

GenesisDescriptor const HARD_CODED_CONFIGS[] = {
    {
      "devnet",
      10000u,
      R"(
{
  "version": 4,
  "accounts": [
    {
      "address": "GTKrC7wEKrDj1FxnnTLd63ZkJH8RQfYr3xoMZ4ZKxxeXNKxsN",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2QGXWiceRkfdj4yE3ZfGdGWjxA6BH5MzLJSfFXySCMv9zPqjgJ",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "NfHwkaKV6zgaSZxKvbJCyZSSe2ngAFpAgyzDh57wzvwckNZcn",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "Qc7zpcPFnfyHWcss3UEZeUFP7q3zWY896zj5Wf7fyLzm9Rmmh",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2g3B1fRWwvquAHyogsks3RDYGJu2yv1s1Vz4C2KB9Qn7nYQWDm",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2Vq1DEU2wx2ZFNygb1raadhPPSNoRKJiSD416484Ec6FD3rVvD",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "dQjjeenuVGYscdEpBF5Vt3BjHoMYG2hfhFEWtD3V2xyRkwmex",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2jYuQTecWL3SHErFLt6wHZNo5t8qzA48M4WWvxNKQjy36o5ire",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "t7vjy7fXxF2fY3p1Qswn5cKpiA9K83kYqSXcosaLJshyQWc6T",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "JU2w5KxC2BP5SDVuLgwsw1hWkVJpCo7PyhkVKpxuziiMPissh",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "QPdfqy8StxiHSXaZsUzu9CgpkbT4joHiZ1pnD3LeuAVfVqgQ4",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "mKeuQnFXfr3w91gRdnhKfnr4xvrgYhwxe4YpZJYkAaBBs5E7H",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2etpvK5eErEGRPFFAUUbwNg4fX2pZN6jedqUc8PPosjfnQ5AWx",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2YSa8bY9ZJCvigRH5DCoTUZQRtKdbKxGddk6EUrrQwRce3mmjp",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2KyyxRZ2kLeUPRdV9TUTEFwgJ54NzEuu1ykDtxpfT7yKcEdNMX",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2NnMckcwiH9DCXJF2Ep9sEPksKMuL1QeXyqobk6ugMDr77jnvb",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2DGc5GBUcYm5ZnHwBPLj7kQYeQdMi6zJic6jYQ1PxsqXmwbV97",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "MuGfogMPh16VkRmUaqjhVSUb3LdyfXxrr5nPf7R5cjK8DwmsR",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "PTc3AWZbQ8MXgKruF8RX4KrkaE2B4nYqu8eGi3Z6mh5E9kGCQ",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "2viTxiS8pga298QYijRsVPyW7R73zdJ8KvP4B4DjTj8zTngnhJ",
      "balance": 0,
      "stake": 1000
    },
    {
      "address": "GKcXpBbuoq3s4HqZfk4DrAzwuEJbh93Z8XCPG4p1P8EM8BdEt",
      "stake": 0,
      "deed": {
        "thresholds": {
          "transfer": 1,
          "amend": 1
        },
        "signees": {
          "iUX84r9dQk4aAGah9zcW5yeiFrjN7qWsoCWTm4CxUv2QNBkG7": 1,
          "2hgzvj6ZP6KmrBYETGEPiu1UhwJx7wUMEu5rACRdcLsQvPYAjb": 1,
          "RX3MEGyhzDtK1C9vDkm2QUfuGv7aGj4kwJVDZsSWyopM3RnQ1": 1,
          "aCa3tCAa47bEVT1g451FATE93thNJdKT2Nn43iofDiciMKocN": 1
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
    "startTime": 1576690831,
    "stakers": [
      {
        "identity": "6rZ4xbThM80BjuvnpvZrF6ErBmttvWHdE1XXegWTHPxOydlg1H/jngtzbd+mVQoZe4wQvsFOOxdOfSNprZvFDQ==",
        "amount": 10000000000000
      },
      {
        "identity": "cFtHB8ntVVdWjZkz0iLOL/VLfygl99Y66kW6rfuUCl+5mGxbG1dbNZsVmPQi0Bu53wtgLO+GVrvuBb0N4s2ocw==",
        "amount": 10000000000000
      },
      {
        "identity": "YgrOaePAqlMBdwu4QZifTNL0rHmqgvf5OffSvGCv8FPbvF5JaHHpAB2pyyOm9NpNMmcO7+IAJL3VL5EmOgMs4w==",
        "amount": 10000000000000
      },
      {
        "identity": "QeAH8ow1aKhf8Uz5lECJfPcTy3qUF+JAdoaH0GOyNGcw8AbzK+LYUIbeyS+sbRB2cCVL5N7Pgg08foAlZMC8uw==",
        "amount": 10000000000000
      },
      {
        "identity": "tx8WsDCjjKx0b+CMQzkYBZuzaty+N7aCYCAhRg3uOgQOLeLsxET4uBjVWrhS/AbKXaRrL8yzIoJoQsGVgzADDA==",
        "amount": 10000000000000
      },
      {
        "identity": "/Zu5bDxNHLiRx4q7G05bi5bpX6dPpin2EEwYUHbZZyJz5M/2gSVwYfM8bOPVkOiMzq4yLeoC8SbwMtQPxvjUQw==",
        "amount": 10000000000000
      },
      {
        "identity": "yC7TQX3RD1bOnjTwtM43Rlv8GrqKtm1OPzVs/LeJko/JkiZfXQQ9H2LCu8PaWkqLdQ4oWlW7tDanA+oAeWz6eg==",
        "amount": 10000000000000
      },
      {
        "identity": "U/knDC8P4g/CY9m+mUzKLRKz19Mw5IFS011ykdMYUbZtEJyYXGlRw3ynh2HBESROur3ejJeKSFtA0IDtamV5QQ==",
        "amount": 10000000000000
      },
      {
        "identity": "YSVxskt3qDiXkl9pOxYzs6QpdvM28NwmohirDUXU+SJdihGCsZiP+JxbCuCFzgk/S2A/LoLqjnckvsuoXUMe0A==",
        "amount": 10000000000000
      },
      {
        "identity": "5OK09P1zJ0m8IMIbGKvx2yb6K54MgJ0Ht1XNJoce1KPvLe32M6jMhE4DXqoNd/WgFPQEe6Ob4R2Tj7D+nmEJew==",
        "amount": 10000000000000
      },
      {
        "identity": "a9I3TjOv0UEjjSuz/UVBBJd1mSjXgcIQA4H1rKFrs26ifdQ2EMIchNH1uRPg61KIm+qcPUtt9sI9vusQ9mpm4A==",
        "amount": 10000000000000
      },
      {
        "identity": "oUSCgkfXVTM9bjonx1i842YPOCUvgy26Mbh1eWxVhpgNErkNndtAClIkRu6e3zzjbsYZ/in7NBByH0OczaV8Og==",
        "amount": 10000000000000
      },
      {
        "identity": "tqNqZWrjEhg/dEgfBSt/h20MNbKqiXKtSwbNJC6cXj/UIwndmkyOVhgPB0xtbOmlqXSb00dGX1tFYmhoTcKRYQ==",
        "amount": 10000000000000
      },
      {
        "identity": "6og/i8y6rPigf5P23OaR9LYy+xSy93hoqWzJwZI4iQapodbGIDAqTjb2+YS1bgq68vHXIvwNr3IBNFKMIJeDiA==",
        "amount": 10000000000000
      },
      {
        "identity": "sPcVCKmxsD3rNftkpHlnVKd0Mry2VJgfHVFYVZ5mGpLvIKjtXKqGJW/SEHii9Y+WG+o31PLlc6KNr5mOYtsW+A==",
        "amount": 10000000000000
      },
      {
        "identity": "wO2ZoMxatnpsVrVgGYDVmuUr0bh4HF6Hl2+BZqCpczvuRUr0z3NiJ4rOz5UCv2VI/jCK5ejcaYuTDHuv9bR0Fw==",
        "amount": 10000000000000
      },
      {
        "identity": "+Pruo5f3aEC8mZX+C7EZSrUAUzdUxM/shyYiMv1TY1z+q/S9/MQ+rWc8dswtAly02zDbKsVh1bl02y4+nP5hqg==",
        "amount": 10000000000000
      },
      {
        "identity": "OMt3hyj/4EmVLEmBTaJK50BXDTbx3/5oPYF31qP+H3/476E3Xr01TfWi3d3MjsXZb+FXk3EG9m47uWTT2Sg1Mg==",
        "amount": 10000000000000
      },
      {
        "identity": "VdX3sBmQO7isllkKRei6ucjRrU/AK+i2JraUSlFXOccH2BXhDRVxpDcRb3EQoNvCq3S7RJkXfrbW/S36tx2/Ag==",
        "amount": 10000000000000
      },
      {
        "identity": "Q1dvsfdtCAkAnQiCB/vlrwjlKGBYuS9sis7WDl7I171HUNDf8Nww/uIadNby2fG7sVAROZpt4ROY0L8ty2p4eQ==",
        "amount": 10000000000000
      }
    ]
  }
}
)"
    }
};

} // namespace

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
  cfg.genesis_file_contents = core::ReadContentsOfFile(genesis_file_path.c_str());

  // evaluate our hard coded genesis files
  auto const &network_name = settings.network_name.value();
  if (cfg.genesis_file_contents.empty() && !network_name.empty())
  {
    for (auto const &genesis : HARD_CODED_CONFIGS)
    {
      if (network_name == genesis.name)
      {
        FETCH_LOG_INFO("ConfigBuilder", "Using ", genesis.name, " configuration");

        cfg.genesis_file_contents = genesis.contents;
        cfg.block_interval_ms     = genesis.block_interval;
        cfg.proof_of_stake        = true;
        break;
      }
    }
  }

  return cfg;
}

}  // namespace fetch
