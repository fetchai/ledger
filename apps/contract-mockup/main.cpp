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

#include "miner.hpp"
#include "contract_register.hpp"
#include "work.hpp"

#include <fstream>
#include <sstream>

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    exit(-9);
  }

  // Reading file
  std::ifstream      file(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  fetch::ledger::DAG dag;
  fetch::ledger::DAGNode node;

  for(auto const &n: dag.nodes())
  {
    node.previous.push_back(n.second.hash);
  }

  node.contents  = "{\"contract\":\"hello.contract\", \"owner\":\"troels\"}";
  dag.Push(node);

  fetch::consensus::ContractRegister cregister;
  if(!cregister.AddContract("0xf232", source))
  {
    std::cout << "Could not attach contract."  << std::endl;
    return 0;
  }

  fetch::consensus::Miner miner(dag);
  fetch::consensus::Work work;
  work.contract_address = "0xf232";
  work.miner = "troels";
  work.nonce = 29188;

  miner.DefineProblem(cregister.GetContract(work.contract_address), work);
  work.score = miner.ExecuteWork(cregister.GetContract(work.contract_address), work);

  return 0;
}
