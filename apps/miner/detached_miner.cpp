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

#include "core/byte_array/encoders.hpp"
#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "core/random/lfg.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/string/trim.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chain/transaction.hpp"
#include "miner/block_optimiser.hpp"
#include "miner/transaction_item.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <unordered_set>

using namespace fetch;
using namespace fetch::optimisers;

ledger::BlockGenerator generator;
std::mt19937           rng(42);

static byte_array::ConstByteArray CreateResource(int64_t value)
{
  std::ostringstream oss;
  oss << "Resource " << value;

  return {oss.str()};
}

static byte_array::ConstByteArray GenerateHash()
{
  byte_array::ByteArray hash;
  hash.Resize(32);

  for (std::size_t i = 0; i < 32; ++i)
  {
    hash[i] = static_cast<uint8_t>(rng() & 0xFF);
  }

  return {hash};
}

static void load_format_a(std::string const &input_file, std::size_t &N, std::size_t &M)
{
  // Loading

  std::fstream f(input_file, std::ios::in);

  if (!f)
  {
    std::cerr << "invalid file" << std::endl;
    exit(-1);
  }

  std::string line;
  std::size_t K;
  f >> N >> M >> K;

  double      total_fee = 0;
  std::size_t id        = 0;

  while (std::getline(f, line))
  {
    fetch::string::Trim(line);
    if (line.size() == 0)
    {
      continue;
    }
    std::stringstream s(line);

    fetch::ledger::TransactionSummary summary;
    summary.transaction_hash = GenerateHash();

    while (s)
    {
      int C = -1, V = -1;
      s >> C >> V;

      auto group = CreateResource(C);

      if (C == -1)
      {
        break;
      }
      if (V == -1)
      {
        std::cerr << "malformed input" << std::endl;
        f.close();
        exit(-1);
      }

      if (C >= int(N))
      {
        std::cerr << "invalid color" << std::endl;
        f.close();
        exit(-1);
      }

      summary.fee += static_cast<uint64_t>(V);
      total_fee += static_cast<uint64_t>(V);

      summary.resources.insert(group);
    }

    std::cout << "Adding transaction: "
              << static_cast<std::string>(byte_array::ToBase64(summary.transaction_hash));
    std::cout << " index: " << id;
    std::cout << " groups: ";
    std::size_t resource_index = 0;
    for (auto const &resource : summary.resources)
    {
      if (resource_index)
      {
        std::cout << ", ";
      }
      std::cout << static_cast<std::string>(resource);
      ++resource_index;
    }
    std::cout << std::endl;

    auto tx = std::make_shared<miner::TransactionItem>(summary, id++);
    generator.PushTransactionSummary(tx, false);
  }
  f.close();
}

static void load_format_b(std::string const &input_file, std::size_t &N, std::size_t &M,
                          std::size_t const &header_format = 0)
{
  // Loading

  std::fstream f(input_file, std::ios::in);

  if (!f)
  {
    std::cerr << "invalid file" << std::endl;
    exit(-1);
  }

  std::string line;
  std::getline(f, line);
  std::stringstream s(line);
  std::size_t       K, lanes = 32;
  switch (header_format)
  {
  case 3:
    M = 1;
    s >> K;
    break;
  case 2:
    M = 1;
    s >> lanes >> K;
    break;
  default:
    s >> lanes >> M >> K;
    break;
  }

  if (N == 0)
  {
    N = lanes;
  }

  std::size_t id = 0;

  double total_fee = 0;
  while (std::getline(f, line))
  {
    fetch::string::Trim(line);
    if (line.size() == 0)
    {
      continue;
    }
    std::stringstream s(line);

    fetch::ledger::TransactionSummary summary;
    summary.transaction_hash = GenerateHash();

    int V = -1;
    s >> V;
    if (V == -1)
    {
      std::cerr << "malformed input" << std::endl;
      f.close();
      exit(-1);
    }

    summary.fee = static_cast<uint64_t>(V);
    total_fee += static_cast<uint64_t>(V);

    while (s)
    {
      int C = -1;
      s >> C;
      if (C == -1)
      {
        break;
      }

      summary.resources.insert(CreateResource(C % int(N)));
    }

    std::cout << "Adding transaction: "
              << static_cast<std::string>(byte_array::ToBase64(summary.transaction_hash));
    std::cout << " index: " << id << " fee: " << summary.fee;
    std::cout << " groups: ";
    std::size_t resource_index = 0;
    for (auto const &resource : summary.resources)
    {
      if (resource_index)
      {
        std::cout << ", ";
      }
      std::cout << static_cast<std::string>(resource);
      ++resource_index;
    }
    std::cout << std::endl;

    auto tx = std::make_shared<miner::TransactionItem>(summary, id++);
    generator.PushTransactionSummary(tx, false);
  }
  f.close();
}

static void PrintSummary(std::size_t const &slice_count)
{
  uint64_t    total_fee = 0;
  std::size_t total_txs = 0;
  for (auto const &e : generator.block_fees())
  {
    total_fee += e;
  }
  for (auto const &slice : generator.block())
  {
    total_txs += slice.size();
  }

  std::size_t const capacity = slice_count * generator.lane_count();
  double const      occupancy_pc =
      (100. * static_cast<double>(generator.block_occupancy())) / static_cast<double>(capacity);

  std::cout << "Fee: " << total_fee << " Txs: " << total_txs << " / " << capacity << " ("
            << occupancy_pc << "%)" << std::endl;
}

int main(int argc, char **argv)
{
  commandline::ParamsParser params;
  params.Parse(argc, argv);

  bool show_help = (params.GetParam<int>("help", 0) == 1);

  if (show_help || (params.arg_size() != 2))
  {
    std::cout << std::endl;

    fetch::commandline::DisplayCLIHeader("Detached Miner");

    std::cout << "Usage: " << argv[0] << " [input] [parameters ...]" << std::endl;
    std::cout << std::endl << std::endl;

    std::cout << "Parameters:" << std::endl;
    std::cout << std::setw(18) << "-slice-count";
    std::cout << std::setw(10) << "[number]";
    std::cout << " slices to be generated for the block.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-reps";
    std::cout << std::setw(10) << "[number]";
    std::cout << " attempts to generate a block.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-batch-size";
    std::cout << std::setw(10) << "[number]";
    std::cout << " transactions considered for each slice.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-explore";
    std::cout << std::setw(10) << "[number]";
    std::cout << " repeated attempts to optimise a single slice.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-sweeps";
    std::cout << std::setw(10) << "[number]";
    std::cout << " simulated annealing parameter specifying runtime.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-b0";
    std::cout << std::setw(10) << "[double]";
    std::cout << " simulated annealing parameter specifying inverse start "
                 "temperature.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-b1";
    std::cout << std::setw(10) << "[double]";
    std::cout << " simulated annealing parameter specifying inverse final "
                 "temperature.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-strategy";
    std::cout << std::setw(10) << "[number]";
    std::cout << " indicates the strategy to pick a batch.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-file-format";
    std::cout << std::setw(10) << "[number]";
    std::cout << " selects the input file format.";
    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Flags:" << std::endl;
    std::cout << std::setw(18) << "-print-results";
    std::cout << std::setw(10) << " ";
    std::cout << " prints results of each block generation process.";
    std::cout << std::endl;

    std::cout << std::setw(18) << "-print-solution";
    std::cout << std::setw(10) << " ";
    std::cout << " prints the best found solution.";
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;

    exit(-1);
  }

  std::size_t lane_count = 0, slice_count;
  std::string input_file = params.GetArg(1);

  std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();

  int         file_format = params.GetParam<int>("file-format", 1);
  double      beta0, beta1;
  std::size_t sweeps, reps, batch_size, explore;
  int         strategy = 0;

  lane_count = params.GetParam<std::size_t>("lane-count", lane_count);

  // Loding file
  switch (file_format)
  {
  case 0:
    load_format_a(input_file, lane_count, slice_count);
    break;
  case 1:
    load_format_b(input_file, lane_count, slice_count);
    break;
  case 2:
    load_format_b(input_file, lane_count, slice_count, 2);
    break;
  case 3:
    load_format_b(input_file, lane_count, slice_count, 3);
    break;
  default:
    std::cerr << "unnknown file format" << std::endl;
    exit(-1);
  }

  // Params
  slice_count = params.GetParam<std::size_t>("slice-count", slice_count);
  reps        = params.GetParam<std::size_t>("reps", 1000);
  batch_size  = params.GetParam<std::size_t>("batch-size", std::size_t(-1));
  explore     = params.GetParam<std::size_t>("explore", 10);
  sweeps      = params.GetParam<std::size_t>("sweeps", 100);
  beta0       = params.GetParam<double>("b0", 0.1);
  beta1       = params.GetParam<double>("b1", 3);
  strategy    = params.GetParam<int>("strategy", 0);

  // Flags
  bool print_stats    = (params.GetParam<int>("print-stats", 0) == 1);
  bool print_solution = (params.GetParam<int>("print-solution", 0) == 1);

  // Solving
  generator.ConfigureAnnealer(sweeps, beta0, beta1);

  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

  batch_size = std::min(generator.unspent_count(), batch_size);

  // Generating solution
  uint64_t best_fee = 0;
  for (std::size_t i = 0; i < reps; ++i)
  {
    generator.Reset();
    generator.GenerateBlock(lane_count, slice_count,
                            static_cast<ledger::BlockGenerator::Strategy>(strategy), batch_size,
                            explore);

    if (print_solution)
    {
      uint64_t total_fee = 0;
      for (auto const &e : generator.block_fees())
      {
        total_fee += e;
      }
      if (total_fee < best_fee)
      {
        best_fee = total_fee;
      }
    }

    if (print_stats)
    {
      PrintSummary(slice_count);
    }
  }

  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

  if (print_solution)
  {
    std::cout << "-- solution --" << std::endl;

    PrintSummary(slice_count);

    std::cout << best_fee << std::endl;

    auto const &block = generator.block();
    std::cout << "N: " << block.size() << std::endl;
    for (auto const &item : block)
    {
      std::cout << "M: " << item.size() << std::endl;
      for (auto const &element : item)
      {
        std::cout << " - : " << element << std::endl;
      }
    }

    // TODO(tfr): generator.PrintSolution();
  }

  std::cout << std::endl;
  std::cout << "# ";

  for (std::size_t i = 0; i < params.arg_size(); ++i)
  {
    std::cout << params.GetArg(i) << " ";
  }

  std::cout << " -sweeps " << sweeps << " -b0 " << beta0 << " -b1 " << beta1 << " -lane-count "
            << lane_count << " -slice-count " << slice_count << std::endl;

  double ts0 = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
  double ts1 = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  std::cout << "# load: " << ts0 * 1000 << " ms, ";
  std::cout << "runtime: " << ts1 * 1000 << " ms, ";
  std::cout << "runtime pr. run: " << ts1 * 1000. / double(reps) << " ms" << std::endl;

  return 0;
}
