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

#include <cstdint>

namespace fetch {

static constexpr uint16_t SERVICE_MUDDLE     = 0;  // reserved
static constexpr uint16_t SERVICE_P2P        = 1001;
static constexpr uint16_t SERVICE_MAIN_CHAIN = 2002;
static constexpr uint16_t SERVICE_LANE       = 3003;
static constexpr uint16_t SERVICE_LANE_CTRL  = 3004;
static constexpr uint16_t SERVICE_EXECUTOR   = 4004;
static constexpr uint16_t SERVICE_DAG        = 4005;

// Common Service Channels
static constexpr uint16_t CHANNEL_RPC = 1;  // for convenience we essentially
                                            // reserve channel 1 of any service
                                            // to be allocated to any
                                            // potential RPC interface

// Muddle Service Channels
static constexpr uint16_t CHANNEL_ROUTING = 1;

// P2P Service Channels

// Main Chain Service Channels
static constexpr uint16_t CHANNEL_NODES = 300;

// RPC Protocol identifiers
static constexpr uint64_t RPC_MAIN_CHAIN = 128;

static constexpr uint64_t RPC_IDENTITY          = 200;
static constexpr uint64_t RPC_STATE             = 201;
static constexpr uint64_t RPC_TX_STORE          = 202;
static constexpr uint64_t RPC_TX_STORE_SYNC     = 203;
static constexpr uint64_t RPC_SLICE_STORE       = 204;
static constexpr uint64_t RPC_SLICE_STORE_SYNC  = 205;
static constexpr uint64_t RPC_CONTROLLER        = 206;
static constexpr uint64_t RPC_EXECUTION_MANAGER = 207;
static constexpr uint64_t RPC_EXECUTOR          = 208;
static constexpr uint64_t RPC_P2P_RESOLVER      = 209;
static constexpr uint64_t RPC_MISSING_TX_FINDER = 210;
static constexpr uint64_t RPC_DAG_STORE_SYNC    = 211;

static constexpr uint64_t CHANNEL_RPC_BROADCAST = 801;
static constexpr uint64_t SERVICE_DAG_BDCAST    = 802;

static constexpr uint16_t CHANNEL_BLOCKS = 2;

}  // namespace fetch
