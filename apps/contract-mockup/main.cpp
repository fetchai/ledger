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

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_register.hpp"

#include <fstream>
#include <sstream>

void LoadDAG(std::string const &filename, fetch::ledger::DAG &dag)
{
  std::fstream data_file(filename, std::ios::in);
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
    for(auto const &n: dag.nodes())
    {
      node.previous.push_back(n.second.hash);
    }

    std::stringstream body;
    body << doc;
    node.contents  = body.str();
    dag.Push(node);

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
    std::cout << "Adding " << doc << std::endl;
    
    // Saving to DAG.
    fetch::ledger::DAGNode node;
    for(auto const &n: dag.nodes())
    {
      node.previous.push_back(n.second.hash);
    }

    std::stringstream body;
    body << doc;
    node.contents  = body.str();
    dag.Push(node);

  }

}

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    std::cerr << "usage ./" << argv[0] << " [dag] [script]" << std::endl;
    exit(-9);
  }

  // Reading the script
  std::ifstream      file(argv[2], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  fetch::ledger::DAG dag;
  LoadDAG(argv[1], dag);

  fetch::consensus::SynergeticContractRegister cregister;
  if(!cregister.CreateContract("0xf232", source))
  {
    std::cout << "Could not attach contract."  << std::endl;
    return 0;
  }

  fetch::consensus::SynergeticMiner miner(dag);
  fetch::consensus::Work work;
  work.contract_name = "0xf232";
  work.miner = "troels";

  std::cout << "Defining problem!" << std::endl;
  if(!miner.DefineProblem(cregister.GetContract(work.contract_name)))
  {
    std::cout << "Could not define problem!" << std::endl;
    exit(-1);
  }

  // Let's mine
  fetch::consensus::WorkRegister wreg;
  std::cout << "Working" << std::endl;
  for(int64_t i = 0; i < 10; ++i) {
    work.nonce = 29188 + i;
    work.score = miner.ExecuteWork(cregister.GetContract(work.contract_name), work);
    wreg.RegisterWork(work);
  }

  wreg.ClearWorkPool( cregister.GetContract(work.contract_name) );

  return 0;
}
