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

#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_register.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <fstream>

//using ::testing::_;

class SynergeticMinerTest : public ::testing::Test
{
public:
  using DAG = fetch::ledger::DAG;
  using SynergeticMiner = fetch::consensus::SynergeticMiner;
  using UniqueMiner = std::unique_ptr< SynergeticMiner >;
  using UniqueDAG = std::unique_ptr< DAG >;

  SynergeticMinerTest() = default;

  void SetUp() override
  {
    std::ifstream      file("./synergetic_test_contract.etch", std::ios::binary);
    if(!file)
    {
      throw std::runtime_error("Could not open contract code.");
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    source_ = ss.str();

    file.close();

    dag_ = UniqueDAG( new DAG() );
    miner_ = UniqueMiner( new SynergeticMiner(*dag_.get()) );
    LoadDAG("./synergetic_test_dag.dag");
  }

  void TearDown() override
  {
    miner_.reset();
    dag_.reset();
  }

  bool Mine() 
  {
    if(!cregister_.CreateContract("fetch.synergetic", source_))
    {
      std::cout << "Could not attach contract." << std::endl;
      return false;
    }

    fetch::consensus::Work work;
    work.contract_name = "fetch.synergetic";
    work.miner = "miner9";

    miner_->AttachContract(cregister_.GetContract(work.contract_name));
    if(!miner_->DefineProblem())
    {
      std::cout << "Could not define problem!" << std::endl;
      return false;
    }

    // Let's mine
    fetch::consensus::WorkRegister wreg;

    fetch::consensus::Work::ScoreType best_score = std::numeric_limits< fetch::consensus::Work::ScoreType >::max();
    for(int64_t i = 0; i < 10; ++i) {
      work.nonce = 29188 + i;
      work.score = miner_->ExecuteWork(work);

      if(work.score < best_score)
      {
        best_score = work.score;
      }
      wreg.RegisterWork(work);
    }

    auto best_work = wreg.ClearWorkPool( cregister_.GetContract(work.contract_name) );

    miner_->DetachContract();

    return  best_work.score == best_score;
  }

private:

  void LoadDAG(std::string const &filename)
  {
    std::fstream data_file(filename, std::ios::in);
    if(!data_file)
    {
      throw std::runtime_error("file could not be opened.");
    }
    uint32_t item_count = uint32_t(-1), bid_count = uint32_t(-1);
    data_file >> item_count >> bid_count;

    bid_count += item_count;

    data_file >> std::ws;

    uint32_t i = 0;
    int j = 0;
    for(; i < item_count; ++i)
    {
      double price;
      std::string id;
      fetch::variant::Variant doc = fetch::variant::Variant::Object();
      data_file >> id >> price >> std::ws;

      doc["type"] = 2;
      doc["id"] = j++;
      doc["agent"] = id;
      doc["price"] = price;

      // Saving to DAG.
      fetch::ledger::DAGNode node;
      for(auto const &n: dag_->nodes())
      {
        node.previous.push_back(n.second.hash);
      }

      std::stringstream body;
      body << doc;
      node.contents  = body.str();
      dag_->Push(node);
    }  

    // #agent_id, #items item0 ... itemN price #exludes exclude0 ... exludeM
    j = 0;
    for(; i < bid_count; ++i)  
    {
      fetch::variant::Variant doc = fetch::variant::Variant::Object();

      uint32_t agent_id, no_items, no_excludes;
      double price;

      data_file >> agent_id >> no_items;
      fetch::variant::Variant bid_on = fetch::variant::Variant::Array(no_items);     

      for(uint32_t j=0; j < no_items; ++j)
      {
        uint32_t item;
        data_file >> item;
        bid_on[j] = item;
      }

      data_file >> price;
      data_file >> no_excludes;
      fetch::variant::Variant excludes = fetch::variant::Variant::Array(no_excludes);

      for(uint32_t j=0; j < no_excludes; ++j)
      {
        uint32_t exclude;      
        data_file >> exclude;
        excludes[j] = exclude;
      }
      doc["id"] = j++;
      doc["type"] = 3;
      doc["agent"] = agent_id;
      doc["price"] = price;
      doc["bid_on"] = bid_on;
      doc["excludes"] = excludes;
      
      // Saving to DAG.
      fetch::ledger::DAGNode node;
      for(auto const &n: dag_->nodes())
      {
        node.previous.push_back(n.second.hash);
      }

      std::stringstream body;
      body << doc;
      node.contents  = body.str();
      dag_->Push(node);
    }
  }


  UniqueMiner miner_;
  UniqueDAG dag_;
  std::string source_;
  fetch::consensus::SynergeticContractRegister cregister_;  
};


TEST_F(SynergeticMinerTest, CheckMinerExecution)
{
  EXPECT_TRUE(Mine());

}
