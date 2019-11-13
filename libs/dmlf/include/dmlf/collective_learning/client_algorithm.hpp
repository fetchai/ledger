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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "dmlf/collective_learning/client_algorithm_controller.hpp"
#include "dmlf/collective_learning/client_params.hpp"
#include "dmlf/update.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/model/sequential.hpp"
#include "ml/utilities/utils.hpp"
#include "ml/utilities/sparse_tensor_utilities.hpp"

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fetch {
namespace dmlf {
namespace collective_learning {

template <class TensorType>
class ClientAlgorithm
{
  using DataType         = typename TensorType::Type;
  using SizeType         = fetch::math::SizeType;
  using VectorTensorType = std::vector<TensorType>;
    using VectorSizeVector         = std::vector<std::vector<SizeType>>;
    using TimestampType    = int64_t;
  using UpdateType       = fetch::dmlf::Update<TensorType>;
  using DataloaderPtrType =
      std::shared_ptr<fetch::ml::dataloaders::DataLoader<TensorType, TensorType>>;
  using GraphPtrType               = std::shared_ptr<fetch::ml::Graph<TensorType>>;
  using OptimiserPtrType           = std::shared_ptr<fetch::ml::optimisers::Optimiser<TensorType>>;
  using ModelPtrType               = std::shared_ptr<fetch::ml::model::Sequential<TensorType>>;
  using AlgorithmControllerType    = ClientAlgorithmController<TensorType>;
  using AlgorithmControllerPtrType = std::shared_ptr<ClientAlgorithmController<TensorType>>;

public:
  ClientAlgorithm(AlgorithmControllerPtrType algorithm_controller, std::string id,
                  ClientParams<DataType> const &client_params,
                  std::shared_ptr<std::mutex>   console_mutex_ptr);

  /**
   * explicit copy constructor because mutexes can't be copied
   * @param other
   */
  ClientAlgorithm(ClientAlgorithm &other)
  {
    id_              = other.id_;
    train_loss_      = other.train_loss_;
    test_loss_       = other.test_loss_;
    train_loss_sum_  = other.train_loss_sum_;
    train_loss_cnt_  = other.train_loss_cnt_;
    model_ptr_       = other.model_ptr_;
    graph_ptr_       = other.graph_ptr_;
    optimiser_ptr_   = other.optimiser_ptr_;
    dataloader_ptr_  = other.dataloader_ptr_;
    round_counter_   = other.round_counter_;
    updates_applied_ = other.updates_applied_;

    params_               = other.params_;
    algorithm_controller_ = other.algorithm_controller_;
  }

  virtual ~ClientAlgorithm() = default;

  virtual void Run();
  void         Train();
  virtual void Test();

  ///////////////////////////
  /// Getters and Setters ///
  ///////////////////////////

  void                                SetModel(ModelPtrType model_ptr);
  VectorTensorType                    GetWeights() const;
  void                                SetWeights(VectorTensorType const &new_weights);
  void                                SetParams(ClientParams<DataType> const &new_params);
  std::string                         GetId() const;
  virtual std::shared_ptr<UpdateType> GetUpdate();
  DataType                            GetLossAverage() const;

protected:
  // Client id (identification name)
  std::string id_;

  // Latest loss
  DataType train_loss_ = fetch::math::numeric_max<DataType>();
  DataType test_loss_  = fetch::math::numeric_max<DataType>();

  DataType train_loss_sum_ = static_cast<DataType>(0);
  SizeType train_loss_cnt_ = 0;

  // Client's own model and mutex to protect its weights
  ModelPtrType       model_ptr_;
  GraphPtrType       graph_ptr_;
  OptimiserPtrType   optimiser_ptr_;
  DataloaderPtrType  dataloader_ptr_;
  mutable std::mutex model_mutex_;

  // Console mutex pointer
  std::shared_ptr<std::mutex> console_mutex_ptr_;

  // Count for number of batches
  SizeType round_counter_   = 0;
  SizeType updates_applied_ = 0;

  ClientParams<DataType> params_;

  virtual VectorSizeVector TranslateUpdate(std::shared_ptr<UpdateType> &new_gradients);

            void DoRound();

  void ClearLossFile();

private:
  std::mutex algorithm_mutex_;

  AlgorithmControllerPtrType algorithm_controller_;

  void AggregateUpdate(VectorTensorType const &gradient, VectorSizeVector const &updated_rows);

  void ApplyUpdates();
};

template <class TensorType>
ClientAlgorithm<TensorType>::ClientAlgorithm(AlgorithmControllerPtrType    algorithm_controller,
                                             std::string                   id,
                                             ClientParams<DataType> const &client_params,
                                             std::shared_ptr<std::mutex>   console_mutex_ptr)
  : id_(std::move(id))
  , console_mutex_ptr_(std::move(console_mutex_ptr))
  , params_(std::move(client_params))
  , algorithm_controller_(std::move(algorithm_controller))
{
  ClearLossFile();
}

template <typename TensorType>
void ClientAlgorithm<TensorType>::SetModel(ModelPtrType model_ptr)
{
  model_ptr_      = std::move(model_ptr);
  optimiser_ptr_  = model_ptr_->optimiser_ptr_;
  dataloader_ptr_ = model_ptr_->dataloader_ptr_;
  graph_ptr_      = model_ptr_->graph_ptr_;
}

template <class TensorType>
void ClientAlgorithm<TensorType>::ClearLossFile()
{
  std::ofstream lossfile("losses_" + id_ + ".csv", std::ofstream::out | std::ofstream::trunc);
  lossfile.close();
}

template <class TensorType>
void ClientAlgorithm<TensorType>::SetParams(
    ClientParams<typename TensorType::Type> const &new_params)
{
  params_ = new_params;
}

template <class TensorType>
std::string ClientAlgorithm<TensorType>::GetId() const
{
  return id_;
}

template <class TensorType>
typename TensorType::Type ClientAlgorithm<TensorType>::GetLossAverage() const
{
  return train_loss_sum_ / static_cast<DataType>(train_loss_cnt_);
}

/**
 * Main loop that runs in thread
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::Run()
{
  FETCH_LOCK(algorithm_mutex_);
  // reset loss count
  train_loss_sum_ = static_cast<DataType>(0);
  train_loss_cnt_ = 0;

  std::ofstream lossfile(params_.results_dir + "losses_" + id_ + ".csv",
                         std::ofstream::out | std::ofstream::app);

  updates_applied_ = 0;
  while (updates_applied_ < params_.max_updates)
  {
    // perform a round of training on this client
    DoRound();

    // Validate loss for logging purpose
    Test();

    // Save loss data
    if (lossfile)
    {
      lossfile << fetch::ml::utilities::GetStrTimestamp() << ", "
               << static_cast<double>(train_loss_) << ", " << static_cast<double>(test_loss_)
               << "\n";
    }

    if (params_.print_loss)
    {
      // Lock console
      FETCH_LOCK(*console_mutex_ptr_);
      std::cout << "Client " << id_ << "\tTraining loss: " << static_cast<double>(train_loss_)
                << "\tTest loss: " << static_cast<double>(test_loss_) << std::endl;
    }
  }

  optimiser_ptr_->IncrementEpochCounter();
  optimiser_ptr_->UpdateLearningRate();

  if (lossfile)
  {
    lossfile << fetch::ml::utilities::GetStrTimestamp() << ", "
             << "STOPPED"
             << "\n";
    lossfile.close();
  }

  if (params_.print_loss)
  {
    // Lock console
    FETCH_LOCK(*console_mutex_ptr_);
    std::cout << "Client " << id_ << " STOPPED" << std::endl;
  }
}

/**
 * Train one round
 * @return training batch loss
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::Train()
{
  dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
  dataloader_ptr_->SetRandomMode(true);

  bool is_done_set = false;

  std::pair<TensorType, std::vector<TensorType>> input;
  input = dataloader_ptr_->PrepareBatch(params_.batch_size, is_done_set);
  {
    FETCH_LOCK(model_mutex_);

    // Set inputs and label
    auto input_data_it = input.second.begin();
    auto input_name_it = params_.input_names.begin();

    while (input_name_it != params_.input_names.end())
    {
      graph_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    graph_ptr_->SetInput(params_.label_name, input.first);

    TensorType loss_tensor = graph_ptr_->ForwardPropagate(params_.error_name);
    train_loss_            = *(loss_tensor.begin());

    train_loss_sum_ += train_loss_;
    train_loss_cnt_++;

    graph_ptr_->BackPropagate(params_.error_name);
  }
  updates_applied_++;
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::Test()
{
  // If test set is not available we run test on whole training set
  if (dataloader_ptr_->IsModeAvailable(fetch::ml::dataloaders::DataLoaderMode::TEST))
  {
    dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TEST);
  }
  else
  {
    dataloader_ptr_->SetMode(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
  }

  // Disable random to run model on whole test set
  dataloader_ptr_->SetRandomMode(false);

  SizeType test_set_size = dataloader_ptr_->Size();

  dataloader_ptr_->Reset();
  bool is_done_set;
  auto test_pair = dataloader_ptr_->PrepareBatch(test_set_size, is_done_set);
  {
    FETCH_LOCK(model_mutex_);

    // Set inputs and label
    auto input_data_it = test_pair.second.begin();
    auto input_name_it = params_.input_names.begin();

    while (input_name_it != params_.input_names.end())
    {
      graph_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    graph_ptr_->SetInput(params_.label_name, test_pair.first);

    test_loss_ = *(graph_ptr_->Evaluate(params_.error_name).begin());
  }
  dataloader_ptr_->Reset();
}

/**
 * @return vector of gradient update values
 */
template <class TensorType>
std::shared_ptr<typename ClientAlgorithm<TensorType>::UpdateType>
ClientAlgorithm<TensorType>::GetUpdate()
{
  FETCH_LOCK(model_mutex_);


  std::vector<std::unordered_set<SizeType>> vector_set=this->graph_ptr_->GetUpdatedRowsReferences();
    std::vector<TensorType> vector_tensor=graph_ptr_->GetGradients();

  // Convert set to vector
  std::vector<std::vector<SizeType>> out_vector;
    std::vector<TensorType> out_tensors;

  for(SizeType i{0};i<vector_set.size();i++)
  {

    out_vector.push_back(std::vector<SizeType>(vector_set.at(i).begin(),vector_set.at(i).end()));
      out_tensors.push_back(fetch::ml::utilities::ToSparse(vector_tensor.at(i),vector_set.at(i)));

  }



    return std::make_shared<UpdateType>(out_tensors, out_vector);
}

/**
 * @return vector of weights that represents the model
 */
template <class TensorType>
std::vector<TensorType> ClientAlgorithm<TensorType>::GetWeights() const
{
  FETCH_LOCK(model_mutex_);
  return graph_ptr_->GetWeightsReferences();
}

/**
 * Rewrites current model with given one
 * @param new_weights Vector of weights that represent model
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::SetWeights(VectorTensorType const &new_weights)
{
  FETCH_LOCK(model_mutex_);

  auto weights_it = new_weights.cbegin();
  for (auto &trainable_lookup : graph_ptr_->trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Trainable<TensorType>>(
        (trainable_lookup.second)->GetOp());
    trainable_ptr->SetWeights(*weights_it);
    ++weights_it;
  }
}

template <class TensorType>
std::vector<std::vector<fetch::math::SizeType>> ClientAlgorithm<TensorType>::TranslateUpdate(
    std::shared_ptr<UpdateType> &new_gradients )
{
  return new_gradients->GetUpdatedRows();
}

/**
 * Perform one round of training. This includes
 * 1. local training
 * 2. push local updates to algorithm controller
 * 3. apply updates received by algorithm controller
 * 4. aggregate and apply updates
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::DoRound()
{
  // Train one batch to create own gradient
  Train();

  // Give latest gradient update to algorithm controller
  algorithm_controller_->PushUpdate(GetUpdate());

  // Sum all gradients provided by algorithm controller
  while (algorithm_controller_->UpdateCount() > 0)
  {
    // get new update from algorithm controller
    auto new_update = algorithm_controller_->template GetUpdate<UpdateType>();

    // Translate and apply update to model
    AggregateUpdate(new_update->GetGradients(),TranslateUpdate(new_update));

    // track number of total updates applied for evaluation
    updates_applied_++;
  }

  // apply aggregated local and remote updates
  ApplyUpdates();

  round_counter_++;
}

///////////////////////
/// private methods ///
///////////////////////

/**
 * Aggregates updates from any source together in the model
 * In this case the updates are gradients aggregated in the graph
 * @param gradient
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::AggregateUpdate(VectorTensorType const &gradient, VectorSizeVector const &updated_rows)
{
//  assert(gradients.size() == graph_ptr_->GetTrainables().size());

            auto grad_it = gradient.begin();
            auto rows_it = updated_rows.begin();

  for (auto &trainable : graph_ptr_->GetTrainables())
  {
    auto weights_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(trainable);
    weights_ptr->AddToGradient(*grad_it,*rows_it);
    ++grad_it;
      ++rows_it;
  }
}

/**
 * Applies previously aggregated updates in the model
 * @tparam TensorType
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::ApplyUpdates()
{
  FETCH_LOCK(model_mutex_);
  optimiser_ptr_->ApplyGradients(params_.batch_size);
  optimiser_ptr_->IncrementBatchCounters(params_.batch_size);
  optimiser_ptr_->UpdateLearningRate();
}

}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
