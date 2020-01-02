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

#include "chain/transaction.hpp"
#include "chain/transaction_layout.hpp"
#include "chain/transaction_layout_rpc_serializers.hpp"
#include "chain/transaction_rpc_serializers.hpp"
#include "core/commandline/params.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "core/filesystem/write_to_file.hpp"
#include "ledger/chain/block_db_record.hpp"
#include "meta/log2.hpp"

#include "storage/object_store.hpp"

#include <fstream>
#include <regex>

#include <dirent.h>

namespace {
using namespace fetch;
using namespace fetch::storage;
using namespace fetch::ledger;
using namespace fetch::core;

struct DIRDeleter;
struct BlockNode;
struct BlockChain;

using DIRPtr          = std::unique_ptr<DIR, DIRDeleter>;
using LaneIdx         = uint64_t;
using BlockStore      = ObjectStore<BlockDbRecord>;
using TxStore         = ObjectStore<chain::Transaction>;
using TxStores        = std::unordered_map<LaneIdx, TxStore>;
using TxStoresPtr     = std::shared_ptr<TxStores>;
using BlockHash       = Block::Hash;
using BlockWeight     = Block::Weight;
using Blocks          = std::unordered_set<BlockHash>;
using BlockStoreCache = std::vector<BlockDbRecord>;
using BlockChains     = std::multiset<BlockChain>;

struct DIRDeleter
{
  void operator()(DIR *ptr)
  {
    if (ptr != nullptr)
    {
      closedir(ptr);
    }
  }
};

template <typename T>
std::string ComposeTxDbFileName(T const &lane_idx, std::string const &suffix = std::string{})
{
  std::ostringstream s;
  s << "node_storage_lane" << std::setfill('0') << std::setw(3) << lane_idx << "_transaction";

  if (!suffix.empty())
  {
    s << "_" << suffix;
  }

  s << ".db";
  return s.str();
}

struct ChainHeadStore
{
  static constexpr char const *    DEFAULT_FILE_NAME{"chain.head.db"};
  static constexpr std::streamsize EXPECTED_HASH_SIZE{32ll};

  std::string const file_name;

  explicit ChainHeadStore(std::string file_name_ = std::string{DEFAULT_FILE_NAME})
    : file_name{std::move(file_name_)}
  {}

  BlockHash GetHead() const
  {
    auto file_content{core::ReadContentsOfFile(file_name.c_str(), EXPECTED_HASH_SIZE + 1ll)};
    if (file_content.size() != EXPECTED_HASH_SIZE)
    {
      return {};
    }

    return file_content.SubArray(0ull, EXPECTED_HASH_SIZE);
  }

  void SetHead(BlockHash const &head) const
  {
    if (head.size() != EXPECTED_HASH_SIZE)
    {
      std::ostringstream s;
      s << "Size of block hash " << head.size() << " differs to expected size "
        << EXPECTED_HASH_SIZE;
      throw std::runtime_error(s.str());
    }

    if (!WriteToFile(file_name.c_str(), head))
    {
      std::ostringstream s;
      s << "Error occured when writing 0x" << head.ToHex() << " to " << file_name;
      throw std::runtime_error(s.str());
    }
  }
};

struct BlockNode
{
  using BlockChildren = Blocks;

  BlockDbRecord db_record;
  BlockChildren children;
  bool          is_block_set;

  BlockNode(BlockDbRecord block, BlockChildren children, bool is_block_set)
    : db_record{std::move(block)}
    , children{std::move(children)}
    , is_block_set{is_block_set}
  {}

  std::size_t TxCount() const
  {
    std::size_t tx_count{0ull};

    if (is_block_set)
    {
      for (auto const &slice : db_record.block.slices)
      {
        tx_count += slice.size();
      }
    }

    return tx_count;
  }
};

struct BlockChain
{
  BlockHash   root{};
  BlockHash   leaf{};
  BlockWeight total_weight;
  std::size_t chain_length;
  std::size_t num_of_all_txs;

  explicit BlockChain(BlockNode const &leaf_)
    : root{leaf_.db_record.hash()}
    , leaf{leaf_.db_record.hash()}
    , total_weight{leaf_.is_block_set ? leaf_.db_record.block.weight : 0ull}
    , chain_length{leaf_.is_block_set ? 1ull : 0ull}
    , num_of_all_txs{leaf_.TxCount()}
  {}

  explicit BlockChain(BlockHash const &block_hash)
    : root{block_hash}
    , leaf{block_hash}
    , total_weight{0ull}
    , chain_length{0ull}  // Intentionally set to 0 because this is for *empty* db record node where
                          // only hash is known (= root, e.g. genesis)
    , num_of_all_txs{0ull}
  {}

  BlockChain(BlockChain const &parent_chain, BlockNode const &leaf_)
    : root{parent_chain.root}
    , leaf{leaf_.db_record.hash()}
    , total_weight{leaf_.db_record.block.weight + parent_chain.total_weight}
    , chain_length{parent_chain.chain_length + 1ull}
    , num_of_all_txs{parent_chain.num_of_all_txs + leaf_.TxCount()}
  {}

  bool operator<(BlockChain const &other) const
  {
    return total_weight < other.total_weight;
  }

  friend std::ostream &operator<<(std::ostream &s, BlockChain const &chain)
  {
    s << "ROOT[" << chain.root.ToHex() << "] ... LEAF[" << chain.leaf.ToHex()
      << "]: weight = " << chain.total_weight << ", chain length = " << chain.chain_length;

    return s;
  }
};

class BlockChainForwardTree
{
  struct RecursionContext
  {
    using Stack = std::deque<RecursionContext>;

    BlockNode const *      node;
    Blocks::const_iterator curr_child_itr;
    BlockChain             chain;

    RecursionContext(BlockNode const &node_, BlockChain const &parent_chain)
      : node{&node_}
      , curr_child_itr{node_.children.cbegin()}
      , chain{parent_chain, node_}
    {}

    explicit RecursionContext(BlockNode const &node_)
      : RecursionContext(node_, BlockChain{node_})
    {}

    RecursionContext(BlockNode const &node_, BlockHash const &block_hash)
      : RecursionContext(node_, BlockChain{block_hash})
    {}

    static BlockChains Recurse(BlockHash const &root, BlockChainForwardTree const &block_tree);

  private:
    static bool RecurseInternal(Stack &stack, BlockChainForwardTree const &block_tree,
                                BlockChains &chains);
  };

public:
  using Tree = std::unordered_map<BlockHash, BlockNode>;

  struct Metadata
  {
    std::size_t num_of_existing_blocks{0ull};
    std::size_t num_of_empty_blocks{0ull};
    Blocks      roots;
  };

  Tree const     tree;
  Metadata const metadata;

  explicit BlockChainForwardTree(BlockStore &block_store)
    : tree{ReadBlockDb(block_store)}
    , metadata{GetMetadata(tree)}
  {}

  /*
   * Functor: Any type of which instance behaves as callable with signature
   * `bool functor(BlockNode const &node, BlockHash const &block_hash)`,
   * where boolean return value gives opportunity for early break from interation.
   */
  template <typename Functor>
  void IterateChainBackward(BlockChain const &chain, Functor &&functor) const
  {
    auto hash{&chain.leaf};
    while (true)
    {
      auto const &node{tree.at(*hash)};
      if (!node.is_block_set)
      {
        break;
      }

      if (!functor(node, *hash))
      {
        break;
      }

      hash = &node.db_record.block.previous_hash;
    }
  }

  void SaveChainToDbStore(BlockChain const &chain, std::string const &suffix) const
  {
    BlockStore repaired_block_store;
    repaired_block_store.New("chain_" + suffix + ".db", "chain_" + suffix + ".index.db");

    IterateChainBackward(chain, [&repaired_block_store](BlockNode const &node, BlockHash const &) {
      repaired_block_store.Set(ResourceID{node.db_record.hash()}, node.db_record);
      return true;
    });

    repaired_block_store.Flush(false);

    ChainHeadStore head_store{"chain_" + suffix + ".head.db"};
    head_store.SetHead(chain.leaf);
  }

  BlockChains FindChains() const
  {
    BlockChains chains;
    for (auto const &root_hash : metadata.roots)
    {
      auto subchains{RecursionContext::Recurse(root_hash, *this)};

      if (!subchains.empty())
      {
        for (auto &chain : subchains)
        {
          chains.insert(chain);
        }
        // chains.insert(subchains.begin(), chains.end());
      }
      else
      {
        // This shall not happen, each root shall have at least one subchain (with root block as the
        // only block in the chain)
        std::cerr << "INCONSISTENCY: NO SubChain(s) for ROOT[0x" << root_hash.ToHex()
                  << "] has/have been found!" << std::endl;
        std::cerr.flush();
      }
    }

    return chains;
  }

  std::tuple<BlockChain, int, std::string> GetHeaviestChain(
      BlockChains const &chains, ChainHeadStore const &chain_head_store) const
  {
    std::ostringstream s;

    if (chains.empty())
    {
      return {BlockChain{BlockHash{}}, -10, "ERROR: No chains found in block db. Exiting."};
    }

    auto const one_of_heaviest_chains{chains.crbegin()};
    auto const num_of_heaviest_chains{chains.count(*one_of_heaviest_chains)};
    if (num_of_heaviest_chains > 1)
    {
      s << "INCONSISTENCY: Found multiple heaviest chains (multiple chains with weight equal to "
           "max. chain weight)."
        << std::endl;
    }

    auto heaviest_chain{chains.cend()};

    auto const chain_head_from_file{chain_head_store.GetHead()};
    auto const block_node_chff{tree.find(chain_head_from_file)};

    BlockChains::const_iterator block_chain_chff{chains.cend()};
    if (block_node_chff == tree.end() || !block_node_chff->second.is_block_set)
    {
      s << "INCONSISTENCY: No corresponding block data found for block hash 0x"
        << chain_head_from_file.ToHex() << " stored in the \"" << chain_head_store.file_name
        << "\" file containing assumed chain head." << std::endl;
    }
    else
    {
      for (auto chain{chains.cbegin()}; chain != chains.cend(); ++chain)
      {
        if (chain->leaf == chain_head_from_file)
        {
          block_chain_chff = chain;
          break;
        }
      }
    }

    if (block_chain_chff == chains.cend())
    {
      s << "INCONSISTENCY: *NO* corresponding CHAIN found for the HEAD block 0x"
        << chain_head_from_file.ToHex() << " stored in the \"" << chain_head_store.file_name
        << "\" file." << std::endl;
    }
    else
    {
      if (block_chain_chff->total_weight == one_of_heaviest_chains->total_weight)
      {
        heaviest_chain = block_chain_chff;
        s << "Heaviest chain corresponds to the HEAD block 0x" << chain_head_from_file.ToHex()
          << " stored in the \"" << chain_head_store.file_name << "\" file." << std::endl;
      }
      else
      {
        s << "INCONSISTENCY: CHAIN corresponding to the HEAD block 0x"
          << chain_head_from_file.ToHex() << " stored in the \"" << chain_head_store.file_name
          << "\" file is *NOT* the heaviest chain." << std::endl;
      }
    }

    if (heaviest_chain == chains.cend())
    {
      // We haven't find the heaviest chain yet.
      if (num_of_heaviest_chains == 1ull)
      {
        heaviest_chain = chains.find(*one_of_heaviest_chains);
      }
      else
      {
        // Trying to recover if possible
        if (block_chain_chff != chains.cend())
        {
          s << "RECOVERY: Picking the heaviest chain using the assumed HEAD block 0x"
            << chain_head_from_file.ToHex() << " stored in the \"" << chain_head_store.file_name
            << "\" file EVEN if it *NOT* the heaviest chain, because there exist *MULTIPLE* "
               "heavier chains which ."
            << std::endl;
          heaviest_chain = block_chain_chff;
        }
        else
        {
          s << "ERROR: *UNABLE* to recover while selecting heviest chain: Assumed HEAD block 0x"
            << chain_head_from_file.ToHex() << " stored in the \"" << chain_head_store.file_name
            << "\" file does *NOT* correspond to any of existing blocks in chain store db, and "
               "there exist multiple heaviest chains."
            << std::endl;
          return {BlockChain{BlockHash{}}, -11, s.str()};
        }
      }
    }

    return {*heaviest_chain, 0, s.str()};
  }

  std::tuple<int, std::string> ValidateChain(BlockChain const &chain) const
  {
    int         err_code{0};
    std::string err_msg;

    uint64_t last_block_number{0ull};
    uint64_t i{0ull};
    IterateChainBackward(chain, [&chain, &err_code, &err_msg, &last_block_number, &i](
                                    BlockNode const &node, BlockHash const &block_hash) {
      // std::cout << "DEBUG{ValidateChain(...)}: last block idx = " << last_block_number << ",
      // iteration = " << i << std::endl;

      if (!node.is_block_set)
      {
        if (block_hash != chain.root)
        {
          err_code = -1;
          std::ostringstream s;
          s << "Block hash = 0x" << block_hash.ToHex()
            << " of node with UNSET block db structure (= technical root of the chain) does NOT "
               "match to expected root hash 0x" +
                   chain.root.ToHex();
          err_msg = s.str();
        }

        return false;
      }

      if (node.db_record.hash() != block_hash)
      {
        err_code = -2;
        err_msg =
            "Block hash stored in block DB structure does not match block hash used as key to "
            "fetch block DB structure.";
        return false;
      }

      if (i > 0)
      {
        if (last_block_number != node.db_record.block.block_number + 1)
        {
          err_code = -3;
          std::ostringstream s;
          s << "Block 0x" << block_hash.ToHex() << " has unexpected block number value "
            << node.db_record.block.block_number << ", expected value is " << last_block_number - 1;
          err_msg = s.str();
          return false;
        }
      }

      last_block_number = node.db_record.block.block_number;
      ++i;

      return true;
    });

    if (last_block_number != 0ull)
    {
      err_code = -4;
      std::ostringstream s;
      s << "The root node of the chain has wrong index " << last_block_number
        << ", expected value is 0.";
      err_msg = s.str();
    }

    return {err_code, err_msg};
  }

private:
  static Tree ReadBlockDb(BlockStore &block_store)
  {
    std::size_t const expected_number_of_blocks{block_store.size()};
    std::cout << "Reading blockchain from db (expected num. of blocks " << expected_number_of_blocks
              << ") ... " << std::endl;

    Tree bch;
    bch.reserve(expected_number_of_blocks);

    std::size_t const num_of_progress_steps{10ull};
    std::size_t const progress_step{(expected_number_of_blocks + num_of_progress_steps - 1) /
                                    num_of_progress_steps};

    std::size_t count{0};
    auto const  end{block_store.end()};
    for (auto it{block_store.begin()}; it != end; ++it)
    {
      BlockNode  new_node{*it, BlockNode::BlockChildren{}, true};
      auto const new_node_hash{new_node.db_record.hash()};
      auto       node_it{bch.find(new_node_hash)};

      if (node_it != bch.cend())
      {
        if (node_it->second.is_block_set)
        {
          // This shall never happen since object data store is supposed to ensure uniqueness in
          // regards of key (hash of the block in this particular case).
          std::cerr << "INCONSISTENCY: Duplicate Block! block hash: 0x" << new_node_hash.ToHex()
                    << std::endl;
          continue;
        }

        node_it->second.db_record    = std::move(new_node.db_record);
        node_it->second.is_block_set = true;
      }
      else
      {
        node_it = bch.emplace(new_node_hash, std::move(new_node)).first;
      }

      auto parent_it{bch.find(node_it->second.db_record.block.previous_hash)};
      if (parent_it != bch.cend())
      {
        parent_it->second.children.insert(new_node_hash);
      }
      else
      {
        // parent_it = bch.emplace(node_it->second.db_record.block.previous_hash,
        // BlockNode{BlockDbRecord{}, BlockNode::BlockChildren{new_node_hash}, false}).first;
        bch.emplace(node_it->second.db_record.block.previous_hash,
                    BlockNode{BlockDbRecord{}, BlockNode::BlockChildren{new_node_hash}, false});
      }

      // TODO(pb): This check is supposed to be compiled in, but it can't, since ledger node sets
      // the `next_hash` to genesis hash for all blocks (at least in release/v0.6.x).
      // if (parent_it->second.db_record.next_hash != new_node_hash)
      //{
      //  std::cerr << "INCONSISTENCY: Parent block (0x" << parent_it->first.ToHex() << ") NEXT hash
      //  0x"
      //  << parent_it->second.db_record.next_hash.ToHex() << " does not match child block hash 0x"
      //  << new_node_hash.ToHex() << std::endl;
      //}

      ++count;
      if (0 == count % progress_step || count == expected_number_of_blocks)
      {
        std::size_t const progress_percent{(count == expected_number_of_blocks)
                                               ? 100ull
                                               : ((count / progress_step) * 100ull) /
                                                     num_of_progress_steps};
        std::cout << progress_percent << "% (processed " << count << " blocks)" << std::endl;
      }
    }

    return bch;
  }

  static Metadata GetMetadata(Tree const &tree)
  {
    Metadata md;

    for (auto const &node : tree)
    {
      if (node.second.is_block_set)
      {
        ++md.num_of_existing_blocks;
      }
      else
      {
        if (node.first != chain::ZERO_HASH)
        {
          ++md.num_of_empty_blocks;
        }

        md.roots.emplace(node.first);
      }
    }

    return md;
  }
};

BlockChains BlockChainForwardTree::RecursionContext::Recurse(
    BlockHash const &root, BlockChainForwardTree const &block_tree)
{
  BlockChains chains;

  Stack stack;
  if (block_tree.tree.count(root) == 0)
  {
    std::cerr << "INCONSISTENCY: Chain ROOT block 0x" << root.ToHex()
              << " does NOT exist in the data storage." << std::endl;
    std::cerr.flush();
    return chains;
  }

  auto const &root_node{block_tree.tree.at(root)};
  if (root_node.is_block_set)
  {
    stack.emplace_back(root_node);
  }
  else
  {
    stack.emplace_back(root_node, root);
  }

  std::size_t const max_possible_num_of_cycles{(block_tree.tree.size() - 1) * 2};
  std::size_t       cycles{0};
  while (RecurseInternal(stack, block_tree, chains))
  {
    ++cycles;
    if (cycles > max_possible_num_of_cycles)
    {
      constexpr char const *const msg{"Reached max theoretical depth of tree traversal."};
      // std::cerr << msg << std::endl;
      throw std::runtime_error(msg);
    }
  }

  return chains;
}

bool BlockChainForwardTree::RecursionContext::RecurseInternal(
    Stack &stack, BlockChainForwardTree const &block_tree, BlockChains &chains)
{
  bool retval{false};

  auto &                                      curr{stack.back()};
  BlockChainForwardTree::Tree::const_iterator child_block_itr;

  if (curr.curr_child_itr != curr.node->children.cend() &&
      (child_block_itr = block_tree.tree.find(*curr.curr_child_itr)) != block_tree.tree.cend())
  {
    stack.emplace_back(child_block_itr->second, curr.chain);

    ++curr.curr_child_itr;
    retval = true;
  }
  else
  {
    if (child_block_itr != block_tree.tree.cend())
    {
      std::cerr << "INCOSISTENCE: CHILD block has not been found: " << curr.curr_child_itr->ToHex()
                << " (PARENT block: " << curr.node->db_record.hash() << ")" << std::endl;
      std::cerr.flush();
    }

    if (curr.node->children.empty())
    {
      chains.emplace(curr.chain);
    }

    stack.pop_back();
  }

  return retval;
}

std::tuple<TxStores, int, std::string> OpenTxDbStores()
{
  TxStores tx_stores;

  DIRPtr dp{opendir(".")};
  if (dp)
  {
    std::regex const rex{"node_storage_lane([0-9]+)_transaction\\.db"};
    std::smatch      match;

    LaneIdx lane_idx_min{std::numeric_limits<LaneIdx>::max()};
    LaneIdx lane_idx_max{0};

    dirent *entry{nullptr};
    while ((entry = readdir(dp.get())) != nullptr)
    {
      std::string const name{entry->d_name};
      if (!std::regex_match(name, match, rex) || match.size() != 2)
      {
        continue;
      }

      std::istringstream reader(match[1].str());
      LaneIdx            idx;
      reader >> idx;

      if (idx > lane_idx_max)
      {
        lane_idx_max = idx;
      }
      else if (idx < lane_idx_min)
      {
        lane_idx_min = idx;
      }

      if (tx_stores.find(idx) != tx_stores.end())
      {
        std::ostringstream s;
        s << "The \"" << name << "\" file with index '" << match[1].str()
          << "` has been already inserted before!";
        return {std::move(tx_stores), -1, s.str()};
      }

      tx_stores[idx].Load(name, ComposeTxDbFileName(match[1].str(), "index"), false);

      // std::cout << "DEBUG: " << name << ": " << match[1].str() << " -> " << idx << std::endl;
    }

    if (lane_idx_min != 0 || (lane_idx_max + 1 - lane_idx_min) != tx_stores.size())
    {
      std::ostringstream s;
      s << "ERROR: Files with \"node_storage_lane[0-9]+_transaction(_index)?\\.db\" name format "
           "have inconsistent(non-continuous) numbering -> there are missing files for one or more "
           "indexes.";
      return {std::move(tx_stores), -2, s.str()};
    }

    if (!meta::IsLog2(tx_stores.size()))
    {
      std::ostringstream s;
      s << "Inferred number of lanes " << tx_stores.size()
        << " (number of file indexes) MUST be power of 2.";
      return {std::move(tx_stores), -3, s.str()};
    }

    // std::cout << "INFO: Inferred number of lanes: " << tx_stores.size() << std::endl;
  }

  return {std::move(tx_stores), 0, ""};
}

void ProcessTransactions(BlockChainForwardTree const &bch, BlockChain const &heaviest_chain,
                         TxStores &tx_stores, TxStoresPtr trimmed_tx_stores,
                         bool const print_missing_txs)
{

  constexpr std::size_t num_of_progress_steps{10ull};
  auto const            num_of_lanes{tx_stores.size()};
  auto const            log2_num_of_lanes{meta::Log2(num_of_lanes)};
  std::size_t const     progress_step{(heaviest_chain.num_of_all_txs + num_of_progress_steps - 1) /
                                  num_of_progress_steps};

  std::size_t tx_count_in_blockchain{0};
  std::size_t last_reported_progress_tx_count{0};
  std::size_t tx_count_missing_accumulated{0};
  std::size_t tx_count_processed{0};
  std::size_t tx_count_stored_in_trimmed_db{0};
  std::size_t count_of_all_tx_in_db{0};

  std::vector<uint64_t> tx_count_missing(tx_stores.size(), 0ull);

  for (auto const &tx_lane_store : tx_stores)
  {
    count_of_all_tx_in_db += tx_lane_store.second.size();
    std::cout << "Lane" << tx_lane_store.first
              << ": Tx Count reported by index file of lane source TX db: "
              << tx_lane_store.second.size() << " TXs" << std::endl;
  }
  std::cout << "Number of ALL transactions stored in source TX db: " << count_of_all_tx_in_db
            << " TXs" << std::endl;

  std::cout << "INFO: Checking Transactions from all blocks ... " << std::endl;
  bch.IterateChainBackward(
      heaviest_chain,
      [&tx_stores, &tx_count_in_blockchain, &last_reported_progress_tx_count, &tx_count_missing,
       &tx_count_missing_accumulated, &tx_count_processed, &tx_count_stored_in_trimmed_db,
       &trimmed_tx_stores, progress_step, print_missing_txs,
       log2_num_of_lanes](BlockNode const &node, BlockHash const & /*block_hash*/) {
        uint64_t slice_idx{0};
        for (auto const &slice : node.db_record.block.slices)
        {
          tx_count_in_blockchain += slice.size();

          uint64_t tx_idx_in_slice{0};
          for (auto const &tx_layout : slice)
          {
            auto const         lane{ResourceID{tx_layout.digest()}.lane(log2_num_of_lanes)};
            chain::Transaction tx;

            bool res{false};
            try
            {
              res = tx_stores[lane].Get(storage::ResourceID{tx_layout.digest()}, tx);
            }
            catch (...)
            {
              if (print_missing_txs)
              {
                std::cerr << "EXCEPTION: Tx fetch from db failed:"
                          << " lane = " << lane << ", tx hash = 0x" << tx_layout.digest().ToHex()
                          << std::endl;
                std::cerr.flush();
              }
            }

            if (!res)
            {
              ++(tx_count_missing[lane]);
              ++tx_count_missing_accumulated;
              if (print_missing_txs)
              {
                std::cerr << "INCONSISTENCY: Tx fetch from db failed:"
                          << " lane = " << lane << ", block[" << node.db_record.block.block_number
                          << "] 0x" << node.db_record.block.hash.ToHex()
                          << ", slice = " << slice_idx
                          << ", tx index in slice = " << tx_idx_in_slice << ", tx hash = 0x"
                          << tx_layout.digest().ToHex() << std::endl;
                std::cerr.flush();
              }
            }
            else if (trimmed_tx_stores)
            {
              ++tx_count_stored_in_trimmed_db;
              (*trimmed_tx_stores)[lane].Set(storage::ResourceID{tx_layout.digest()}, tx);
            }

            ++tx_count_processed;
            ++tx_idx_in_slice;
          }
          ++slice_idx;
        }

        if (node.db_record.block.block_number == 0 ||
            (tx_count_in_blockchain > last_reported_progress_tx_count &&
             (tx_count_in_blockchain - last_reported_progress_tx_count >= progress_step ||
              tx_count_in_blockchain >= num_of_progress_steps * progress_step)))
        {
          last_reported_progress_tx_count = tx_count_in_blockchain;
          std::size_t const progress_percent{
              (node.db_record.block.block_number == 0)
                  ? 100ull
                  : ((tx_count_in_blockchain / progress_step) * 100ull) / num_of_progress_steps};
          std::cout << progress_percent << "%"
                    << " (processed up to " << node.db_record.block.block_number
                    << " block INDEX going backwards from tip,"
                    << " missing/failed TX count " << tx_count_missing_accumulated << " (from "
                    << tx_count_processed << " TXs processed so far)." << std::endl;
        }

        return true;
      });

  if (trimmed_tx_stores)
  {
    for (auto &store : *trimmed_tx_stores)
    {
      store.second.Flush(false);
      std::cout << "Lane" << store.first
                << ": Number of transactions stored in trimmed lane Tx db: "
                << tx_count_stored_in_trimmed_db << " TXs" << std::endl;
    }

    std::cout << "Number of ALL transactions stored in trimmed Tx db: "
              << tx_count_stored_in_trimmed_db << " TXs" << std::endl;
  }

  std::cout << "done." << std::endl;

  if (tx_count_in_blockchain > count_of_all_tx_in_db)
  {
    std::cerr << "INCONSISTENCY: Less transactions present in source db store "
              << count_of_all_tx_in_db << " than transactions required by block-chain "
              << tx_count_in_blockchain << std::endl;
  }

  std::size_t lane{0};
  for (auto const &count : tx_count_missing)
  {
    if (count > 0)
    {
      std::cerr << "INCONSISTENCY:"
                << " Lane" << lane << " Tx db store is missing " << count
                << " transactions required by block-chain" << std::endl;
    }
    ++lane;
  }
}

}  // namespace

int main(int argc, char **argv)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  bool print_missing_txs{false};
  bool create_trimmed_tx_store{false};
  bool create_repaired_block_store{false};

  commandline::Params parser{};
  parser.description(
      "Tool for consistency check & analysis of fetch block-chain & transaction storage files.");
  parser.add(print_missing_txs, "print-missing-txs",
             "Print transactions required by block-chain but missing in tx store.", false);
  parser.add(create_repaired_block_store, "repair-block-db",
             "Create repaired Blockchain db store containing only necessary & valid blocks. The "
             "repair creates fresh chain HEAD file.",
             false);
  parser.add(create_trimmed_tx_store, "trim-tx-db",
             "Create trimmed TX db store containing only such TXs which are required by "
             "block-chain & exist in original TX db store.",
             false);
  parser.Parse(argc, argv);

  BlockStore block_store;
  block_store.Load("chain.db", "chain.index.db", false);

  std::cout << "Blocks count reported by block db store: " << block_store.size() << std::endl;

  BlockChainForwardTree const bch{block_store};
  std::cout << "Count of EXISTING blocks in reconstructed blockchain tree: "
            << bch.metadata.num_of_existing_blocks << std::endl;
  std::cout << "Count of EMPTY blocks in reconstructed blockchain tree: "
            << bch.metadata.num_of_empty_blocks << std::endl;

  auto const &roots{bch.metadata.roots};
  std::cout << "No. of roots in reconstructed blockchain tree: " << roots.size() << std::endl;

  auto const chains{bch.FindChains()};
  std::cout << "No. of chains found in reconstructed blockchain tree: " << chains.size()
            << std::endl;

  if (!chains.empty())
  {
    std::cout << "List of chains found in reconstructed blockchain tree:" << std::endl;
    for (auto const &chain : chains)
    {
      std::cout << "Chain: " << chain << std::endl;
    }
    std::cout << "End of the list." << std::endl;
  }

  int         err{0};
  std::string err_msg;
  BlockChain  heaviest_chain{BlockHash{}};
  std::tie(heaviest_chain, err, err_msg) = bch.GetHeaviestChain(chains, ChainHeadStore{});

  if (err < 0)
  {
    std::cerr << err_msg;
    return err;
  }
  std::cout << err_msg;

  std::cout << "Heaviest Chain: " << heaviest_chain << std::endl;

  std::tie(err, err_msg) = bch.ValidateChain(heaviest_chain);
  if (err < 0)
  {
    std::cerr << err_msg << std::endl;
    return -6;
  }

  if (create_repaired_block_store)
  {
    bch.SaveChainToDbStore(heaviest_chain, "repaired");
  }

  TxStores tx_stores;
  std::tie(tx_stores, err, err_msg) = OpenTxDbStores();
  if (err < 0)
  {
    std::cerr << err_msg << std::endl;
    return err;
  }

  TxStoresPtr trimmed_tx_stores{};
  if (create_trimmed_tx_store)
  {
    trimmed_tx_stores = std::make_shared<TxStores>();
    trimmed_tx_stores->reserve(tx_stores.size());
    for (uint64_t lane_idx{0ull}; lane_idx < tx_stores.size(); ++lane_idx)
    {
      (*trimmed_tx_stores)[lane_idx].New(ComposeTxDbFileName(lane_idx, "trimmed"),
                                         ComposeTxDbFileName(lane_idx, "index_trimmed"));
    }
  }

  ProcessTransactions(bch, heaviest_chain, tx_stores, trimmed_tx_stores, print_missing_txs);

  return EXIT_SUCCESS;
}
