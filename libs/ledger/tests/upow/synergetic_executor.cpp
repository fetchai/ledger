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

#include "ledger/upow/synergetic_executor.hpp"
#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_register.hpp"
#include "core/random/lfg.hpp"
#include "math/bignumber.hpp"

#include "mock_storage_unit.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <fstream>

class SynergeticExecutorTest : public ::testing::Test
{
public:
  using SynergeticExecutor    = fetch::consensus::SynergeticExecutor;
  using DAG                   = fetch::ledger::DAG;
  using Block                 = fetch::ledger::Block;
  using FakeChain             = std::vector< Block >;
  using SynergeticMiner       = fetch::consensus::SynergeticMiner;

  using UniqueExecutor        = std::unique_ptr< SynergeticExecutor >;
  using UniqueDAG             = std::unique_ptr< DAG >;
  using UniqueBlock           = std::unique_ptr< Block > ;  
  using UniqueBlockChain      = std::unique_ptr< FakeChain > ;
  using UniqueMiner           = std::unique_ptr< SynergeticMiner >;

  using FakeStorageUnitPtr    = std::unique_ptr<FakeStorageUnit>;
  using Address               = fetch::byte_array::ConstByteArray;
  using ContractRegister      = fetch::consensus::SynergeticContractRegister;
  using Work                  = fetch::consensus::Work;
  using RandomGenerator       = fetch::random::LaggedFibonacciGenerator<>;
  using UniqueCertificate     = std::unique_ptr<fetch::crypto::ECDSASigner>;

  using ConstByteArray        = fetch::byte_array::ConstByteArray;
  using ContractName          = fetch::byte_array::ConstByteArray;
  using ContractAddress       = fetch::storage::ResourceAddress;  
  using Identifier            = fetch::ledger::Identifier;

  char const* CONTRACT_NAME = "zS6wg3ybTJYsItL/i1RBt7RYBYh/BR05vxM7WDuvaGA=.Z+ZQSog6NcP8LMGGpFHHWUeXRVXJT9AXZPvFWdrwT6iixAn7Q7KKKtiSviPWvPmT7KdKNbb8vs4oqg85PMsjaw==.synergetic";

  SynergeticExecutorTest() = default;

  void SetUp() override
  {
    storage_     = std::make_unique<FakeStorageUnit>();
    dag_ = UniqueDAG( new DAG() );
    executor_ = UniqueExecutor( new SynergeticExecutor(*dag_.get(), *storage_.get()) );    
    chain_ = UniqueBlockChain( new FakeChain() );
    miner_ = UniqueMiner( new SynergeticMiner(*dag_.get()) );
    certificate_ = std::make_unique<fetch::crypto::ECDSASigner>();    

//    std::cout << fetch::byte_array::ToBase64(certificate_->identity().identifier()) << std::endl;
    std::cout << fetch::byte_array::ToBase64(fetch::crypto::Hash<fetch::crypto::SHA256>("xxx")) << std::endl;
    // Preparing genesis block
    Block next_block;
    next_block.body.previous_hash = "genesis";
    next_block.body.block_number  = 0;
    next_block.body.miner         = "unkown";
    next_block.body.dag_nodes     = {};

    chain_->push_back(next_block);

    // Adding contract
    std::string source;

    std::ifstream      file("./synergetic_test_contract.etch", std::ios::binary);
    if(!file)
    {
      throw std::runtime_error("Could not open contract code.");
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    source = ss.str();

    // Adding contract to stroge
    Identifier contract_id;
    if (!contract_id.Parse(CONTRACT_NAME))
    {
      std::cerr << "Could not parse contract name" << std::endl;
      TearDown();
      exit(-1);
    }

    // create the resource address for the contract
    ContractAddress contract_address = fetch::ledger::SmartContractManager::CreateAddressForContract(contract_id);    
    fetch::serializers::ByteArrayBuffer adapter;
    adapter << static_cast<ConstByteArray>(source);
    storage_->Set(contract_address, adapter.data());

    if(GetContract() != source)
    {
      std::cout << "Contract not correctly retreived." << std::endl;
      TearDown();
      exit(-1);

    }

    // Testing contract creation
    if(!cregister_.CreateContract(CONTRACT_NAME, source))
    {
      std::cout << "Could not create contract." << std::endl;
      TearDown();
      exit(-1);
    }

    // Clearing the register
    cregister_.Clear();

    random_.Seed(42);
  }

  std::string GetContract()
  {
    Identifier contract_id;
    if (!contract_id.Parse(CONTRACT_NAME))
    {
      std::cerr << "Could not parse contract name" << std::endl;
      TearDown();
      exit(-1);
    }

    // create the resource address for the contract
    ContractAddress contract_address = fetch::ledger::SmartContractManager::CreateAddressForContract(contract_id);    

    auto const result = storage_->Get(contract_address);    
    if (result.failed)
    {
      std::cout << "Failed to get contract" << std::endl;
      TearDown();
      exit(-1);
    }

    ConstByteArray source; 
    fetch::serializers::ByteArrayBuffer adapter{result.document};
    adapter >> source;
    return static_cast<std::string>(source);
  }

  void TearDown() override
  {
    certificate_.reset();
    miner_.reset();
    chain_.reset();
    executor_.reset();
    dag_.reset();
    storage_.reset();    
  }

  void ExecuteRound(uint64_t N = 100, uint64_t mine_every = 5)
  {
    if(!cregister_.HasContract(CONTRACT_NAME))
    {
      if(!cregister_.CreateContract(CONTRACT_NAME, GetContract()))
      {
        std::cout << "Could not add contract" << std::endl;
      }
    }
    // Data is generated
    std::cout << "GENERATING DATA!" << std::endl;
    GenerateDAGData(N, mine_every);

    // The remaining bits should work with out the contract register
    cregister_.Clear();

    std::cout << "MINING!" << std::endl;
    // Block is mined
    MakeBlock();

    std::cout << "EXECUTING!" << std::endl;
    std::cout << "EXECUTING!" << std::endl << std::endl;
    // Block is executed
    ExecuteBlock();
    std::cout << std::endl;
    std::cout << "DONE EXECUTING!" << std::endl;
    std::cout << "DONE EXECUTING!" << std::endl << std::endl;    
  }

  UniqueDAG&          dag() { return dag_; }
  uint64_t random() { return random_(); }
  UniqueBlockChain&   chain() { return chain_; }
private:
  void GenerateDAGData(uint64_t N = 100, uint64_t mine_every = 5)
  {
    std::cout << "Adding nodes"<< std::endl;

    for(uint32_t i=0; i < N; ++i)
    {
      if( (i % mine_every) == 0 )
      {
        // Mining solutions to the data submitted in the last round
        std::cout << " --- > uPoW-ing for block "  << std::endl;
        auto work = Mine();

        // Keeping the old segment for later testing.
        auto old_segment = dag_->ExtractSegment(chain_->back());

        // Testing that work was correctly mined
        miner_->AttachContract(*storage_, cregister_.GetContract(work.contract_name));
        miner_->DefineProblem();
        auto test_score = miner_->ExecuteWork(work);
        EXPECT_EQ(test_score, work.score);
        miner_->DetachContract();
        std::cout << "WORK MINED WORK MINED WORK MINED WORK MINED WORK MINED WORK MINED " << std::endl;
        std::cout << test_score << std::endl;
        std::cout << fetch::byte_array::ToBase64(work.miner) << std::endl;        
        std::cout << fetch::byte_array::ToBase64(work.nonce) << std::endl;
        std::cout << fetch::byte_array::ToBase64(work()) << std::endl;
        std::cout << "Based on DAG segment: " << miner_->block_number() << " " << work.block_number << std::endl;
        for(auto &t: old_segment)
        {
          std::cout << " - " << fetch::byte_array::ToBase64(t.hash) << std::endl;
        }
        std::cout << "END END END END END END END " << std::endl;
        // Business logic: Storing the work in the DAG
        fetch::ledger::DAGNode node;
        node.type = fetch::ledger::DAGNode::WORK;

        node.SetObject(work);
        node.contract_name = work.contract_name;
        node.identity = certificate_->identity();
        dag_->SetNodeReferences(node);

        node.Finalise();
        EXPECT_TRUE(certificate_->Sign(node.hash));

        node.signature = certificate_->signature();
        dag_->Push(node);


        // Testing that nodes in two segments are the same
        auto new_segment = dag_->ExtractSegment(chain_->back());        
        EXPECT_EQ( old_segment.size(), new_segment.size() );
        EXPECT_EQ( old_segment, new_segment);

        // Testing that work is correctly deserialised
        Work work2;
        node.GetObject(work2);
        work2.contract_name = node.contract_name;
        work2.miner = node.identity.identifier();

        EXPECT_EQ(work, work2);

        // Testing that work is reproducible
        miner_->AttachContract(*storage_, cregister_.GetContract(work2.contract_name));
        miner_->DefineProblem();
        auto test_score2 = miner_->ExecuteWork(work2);
        EXPECT_EQ(test_score2, work.score);
        miner_->DetachContract();        

      }
      else
      {
        std::cout << " ---- > Generating Node!" << std::endl;
        // Generating data in this round
        if(!miner_->AttachContract(*storage_, cregister_.GetContract(CONTRACT_NAME)))
        {
          std::cout << "Failed attaching the contract in Generate DAG" << std::endl; 
          TearDown();
          exit(-1);
        }

        // Creating test node
        auto node = miner_->CreateDAGTestData(static_cast<int32_t>(chain_->size()), static_cast<int64_t>( random_() & ((1ul<<31) - 1ul ) ));
        miner_->DetachContract();


        // Adding it to the DAG
        node.contract_name = CONTRACT_NAME;
        dag_->SetNodeReferences(node);

        // Finalising node    
        node.identity = certificate_->identity();
        node.Finalise();
        if(!certificate_->Sign(node.hash))
        {
          throw std::runtime_error("Signing failed");
        }
        node.signature = certificate_->signature();

        // And pushing it to the DAG
        dag_->Push(node);
      }
    }
  }

  void MakeBlock()
  {
    auto current_block = chain_->back();

    Block next_block;
    next_block.body.previous_hash = current_block.body.hash;
    next_block.body.block_number  = current_block.body.block_number + 1;
    next_block.body.miner         = "unkown";
    next_block.body.dag_nodes     = dag_->UncertifiedTipsAsVector();
    std::cout << "Making block: " << next_block.body.block_number <<  std::endl;    
    chain_->push_back(next_block);
  }
  
  enum ExecuteStatus
  {
    SUCCESS = 0,
    REJECT_BLOCK
  };

  ExecuteStatus ExecuteBlock()
  {
    using PStatus = SynergeticExecutor::PreparationStatusType;

    // Ignoring genesis block as no work was done prior to this
    if(chain_->size() < 2)
    {
      return ExecuteStatus::SUCCESS;
    }

    std::cout << "Executing block: ";
    std::cout << chain_->back().body.block_number;
    std::cout << std::endl;

    auto current_block = chain_->back();
    auto previous_block = chain_->at(chain_->size() - 2);

    // Testing executor - preparing work
    if(PStatus::SUCCESS != executor_->PrepareWorkQueue(previous_block, current_block)) 
    {
      std::cout << "FAILED TO PREPARE!" << std::endl;
      return ExecuteStatus::REJECT_BLOCK;
    }

    // Executing work
    if(!executor_->ValidateWorkAndUpdateState())
    {
      std::cout << "FAILED TO VALIDATE!" << std::endl;
      return ExecuteStatus::REJECT_BLOCK;
    }

    return ExecuteStatus::SUCCESS;
  }

  Work Mine(int64_t search_length = 10) 
  {
    fetch::consensus::Work work;    
    work.contract_name = CONTRACT_NAME;
    work.miner = certificate_->identity().identifier();
    work.block_number = chain_->back().body.block_number;

    if(!miner_->AttachContract(*storage_, cregister_.GetContract(work.contract_name)))
    {
      std::cout << "Failed attaching the contract in Mine" << std::endl;
      TearDown();
      exit(-1);
    }

    // Ensuring that we are extracting the right part of the DAG
    if(chain_->size() > 0)
    {
      miner_->SetBlock(chain_->back());
      dag_->SetNodeTime(chain_->back());
    }
    else
    {
      std::cerr << "Genesis block is not present" << std::endl;
      exit(-1);
    }

    // Defining the problem we mine
    if(!miner_->DefineProblem())
    {
      miner_->DetachContract();
      std::cout << "Could not define problem!" << std::endl;
      TearDown();      
      exit(-1);
    }

    // Let's mine
    fetch::math::BigUnsigned nonce(29188);
    fetch::consensus::WorkRegister wreg;
    fetch::consensus::Work::ScoreType best_score = std::numeric_limits< fetch::consensus::Work::ScoreType >::max();
    for(int64_t i = 0; i < search_length; ++i) {      
      work.nonce = nonce.Copy();
      work.score = miner_->ExecuteWork(work);

      if(work.score < best_score)
      {
        best_score = work.score;
      }
      
      ++nonce;
      wreg.RegisterWork(work);
    }

    miner_->DetachContract();

    // Returning the best work from this round
    return wreg.ClearWorkPool( cregister_.GetContract(work.contract_name) );
  }


  FakeStorageUnitPtr storage_;
  UniqueDAG          dag_;
  UniqueExecutor     executor_;
  UniqueBlockChain   chain_;
  UniqueMiner        miner_;  
  ContractRegister   cregister_;  
  RandomGenerator    random_;
  UniqueCertificate  certificate_;
};


TEST_F(SynergeticExecutorTest, CheckMiningFlow)
{
  std::vector< uint64_t > dag_counters;
  dag_counters.push_back(0); // 0 dag nodes before genesis
  uint64_t total = 1; // 1 becuase of Genesis
  uint64_t rounds = 3;

  // Testing live execution for a number of rounds
  for(std::size_t i=0; i < rounds; ++i)
  {
    uint64_t N = 5 + 2*i; //random() % 15;
    dag_counters.push_back(N);
    total += N;
    std::cout << "Adding " << N << " nodes" << std::endl;
    ExecuteRound(N);
    EXPECT_TRUE(dag()->size() == total);    
  }  

  // Verifying DAG certification
  auto &ch = *chain();
  for(std::size_t i=0; i < ch.size(); ++i)
  {
    auto block = ch[i];
    auto segment = dag()->ExtractSegment(block);
    std::cout << "Block " << block.body.block_number << " : " << segment.size() << " vs " <<  dag_counters[i] << " DAG nodes" << std::endl;    

    for(auto &s: segment)
    {
      std::cout << fetch::byte_array::ToBase64(s.hash) <<" : " << s.type << " " << s.timestamp << std::endl;
      if(s.type == fetch::ledger::DAGNode::WORK)      
      {
        fetch::consensus::Work work;
        s.GetObject(work);
        std::cout <<"   -> " << work.score << " " << fetch::byte_array::ToBase64(work.nonce) << " " << work.block_number << std::endl;
        EXPECT_EQ(block.body.block_number - 1, work.block_number);
      }
      else
      {
        std::cout << "   -> " << s.contents << std::endl;
      }
    }


    std::cout << " --- " << std::endl;    
    EXPECT_TRUE(segment.size() == dag_counters[i]);
  }  
}