#pragma once
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "dmlf/collective_learning/client_algorithm_controller.hpp"
#include "dmlf/collective_learning/client_params.hpp"
#include "dmlf/deprecated/update.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/model/sequential.hpp"
#include "ml/utilities/sparse_tensor_utilities.hpp"
#include "ml/utilities/utils.hpp"

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
  using VectorSizeVector = std::vector<std::vector<SizeType>>;
  using TimestampType    = int64_t;
  using UpdateType       = fetch::dmlf::deprecated_Update<TensorType>;
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
    id_                         = other.id_;
    train_loss_                 = other.train_loss_;
    test_loss_                  = other.test_loss_;
    train_loss_sum_             = other.train_loss_sum_;
    train_loss_cnt_             = other.train_loss_cnt_;
    model_ptr_                  = other.model_ptr_;
    graph_ptr_                  = other.graph_ptr_;
    optimiser_ptr_              = other.optimiser_ptr_;
    dataloader_ptr_             = other.dataloader_ptr_;
    batch_counter_              = other.batch_counter_;
    epoch_counter_              = other.epoch_counter_;
    update_counter_             = other.update_counter_;
    updates_applied_this_round_ = other.updates_applied_this_round_;

    params_               = other.params_;
    algorithm_controller_ = other.algorithm_controller_;

    input_names_ = other.input_names_;
    label_name_  = other.label_name_;
    error_name_  = other.error_name_;
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
  DataType train_loss_    = fetch::math::numeric_max<DataType>();
  DataType test_loss_     = fetch::math::numeric_max<DataType>();
  DataType test_accuracy_ = DataType{0};

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

  // Counters
  SizeType batch_counter_  = 0;  // counts batches of own dataset processed
  SizeType epoch_counter_  = 0;  // counts epochs (complete passes through own dataset)
  SizeType update_counter_ = 0;  // counts updates (local batches + updates from peers)

  SizeType updates_applied_this_round_ = 0;
  SizeType epochs_done_this_round_     = 0;

  ClientParams<DataType> params_;

  virtual VectorSizeVector TranslateUpdate(std::shared_ptr<UpdateType> &new_gradients);

  void TrainAndApplyUpdates();

  void ClearLossFile();

  std::vector<std::string> input_names_;
  std::string              label_name_;
  std::string              error_name_;

private:
  std::mutex algorithm_mutex_;

  AlgorithmControllerPtrType algorithm_controller_;

  void AggregateUpdate(VectorTensorType const &gradients);
  void AggregateSparseUpdate(VectorTensorType const &gradients,
                             VectorSizeVector const &updated_rows);

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
  input_names_    = {model_ptr_->InputName()};
  label_name_     = model_ptr_->LabelName();
  error_name_     = model_ptr_->ErrorName();
}

template <class TensorType>
void ClientAlgorithm<TensorType>::ClearLossFile()
{
  mkdir(params_.results_dir.c_str(), 0777);
  std::string   results_file = params_.results_dir + "/losses_" + id_ + ".csv";
  std::ofstream lossfile(results_file, std::ofstream::out | std::ofstream::trunc);
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

  std::ofstream lossfile(params_.results_dir + "/losses_" + id_ + ".csv",
                         std::ofstream::out | std::ofstream::app);

  updates_applied_this_round_ = 0;
  epochs_done_this_round_     = 0;
  while ((updates_applied_this_round_ < params_.max_updates) &&
         (epochs_done_this_round_ < params_.max_epochs))
  {
    // perform a round of training on this client
    TrainAndApplyUpdates();

    // Validate loss for logging purpose
    Test();

    // Save loss data
    if (lossfile)
    {
      lossfile << fetch::ml::utilities::GetStrTimestamp() << ", "
               << static_cast<double>(train_loss_) << ", " << static_cast<double>(test_loss_)
               << ", " << epoch_counter_ << ", " << update_counter_ << ", " << batch_counter_
               << "\n";
      lossfile.flush();
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
    lossfile << "End_of_round: " << fetch::ml::utilities::GetStrTimestamp()
             << " Epochs: " << epoch_counter_ << " Loss: " << static_cast<double>(train_loss_)
             << " Test_loss: " << static_cast<double>(test_loss_) << " Updates: " << update_counter_
             << " Batches: " << batch_counter_ << " Test_accuracy: " << test_accuracy_ << "\n";
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

  bool dataloader_is_done_ = false;

  std::pair<TensorType, std::vector<TensorType>> input;
  input = dataloader_ptr_->PrepareBatch(params_.batch_size, dataloader_is_done_);
  {
    FETCH_LOCK(model_mutex_);

    // Set inputs and label
    auto input_data_it = input.second.begin();
    auto input_name_it = input_names_.begin();

    while (input_name_it != input_names_.end())
    {
      graph_ptr_->SetInput(*input_name_it, *input_data_it);
      ++input_name_it;
      ++input_data_it;
    }
    graph_ptr_->SetInput(label_name_, input.first);

    TensorType loss_tensor = graph_ptr_->ForwardPropagate(error_name_);
    train_loss_            = *(loss_tensor.begin());

    train_loss_sum_ += train_loss_;
    train_loss_cnt_++;

    graph_ptr_->BackPropagate(error_name_);
  }

  if (dataloader_is_done_)
  {
    epochs_done_this_round_++;
    epoch_counter_++;
  }
  batch_counter_++;
  updates_applied_this_round_++;
  update_counter_++;
}

/**
 * Run model on test set to get test loss
 * @param test_loss
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::Test()
{
  if (dataloader_ptr_->IsModeAvailable(fetch::ml::dataloaders::DataLoaderMode::TEST))
  {
    {
      FETCH_LOCK(model_mutex_);

      typename ml::model::Model<TensorType>::DataVectorType results =
          model_ptr_->Evaluate(fetch::ml::dataloaders::DataLoaderMode::TEST);

      test_loss_ = results.at(0);

      if (results.size() == 2)
      {
        test_accuracy_ = results.at(1);
      }
      else if (results.size() > 2)
      {
        throw fetch::ml::exceptions::NotImplemented(
            "More metrics configured for model than "
            "ClientAlgorithm knows how to process.");
      }
    }
  }
}

/**
 * @return Update containing vector of sparse gradient tensors and vector of updated rows
 */
template <class TensorType>
std::shared_ptr<typename ClientAlgorithm<TensorType>::UpdateType>
ClientAlgorithm<TensorType>::GetUpdate()
{
  FETCH_LOCK(model_mutex_);
  return std::make_shared<UpdateType>(graph_ptr_->GetGradients());
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

  std::vector<typename ml::Graph<TensorType>::TrainablePtrType> trainables =
      graph_ptr_->GetTrainables();

  auto weights_it = new_weights.cbegin();
  for (auto &trainable_ptr : trainables)
  {
    trainable_ptr->SetWeights(*weights_it);
    ++weights_it;
  }
}

template <class TensorType>
std::vector<std::vector<fetch::math::SizeType>> ClientAlgorithm<TensorType>::TranslateUpdate(
    std::shared_ptr<UpdateType> &new_gradients)
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
void ClientAlgorithm<TensorType>::TrainAndApplyUpdates()
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
    if (new_update->GetUpdatedRows().empty())
    {
      AggregateUpdate(new_update->GetGradients());
    }
    else
    {
      AggregateSparseUpdate(new_update->GetGradients(), TranslateUpdate(new_update));
    }

    // track number of total updates applied for evaluation
    updates_applied_this_round_++;
    update_counter_++;
  }

  // apply aggregated local and remote updates
  ApplyUpdates();
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
void ClientAlgorithm<TensorType>::AggregateUpdate(VectorTensorType const &gradients)
{
  assert(gradients.size() == graph_ptr_->GetTrainables().size());
  auto grad_it = gradients.begin();
  for (auto &trainable : graph_ptr_->GetTrainables())
  {
    auto weights_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(trainable);
    weights_ptr->AddToGradient(*grad_it);
    ++grad_it;
  }
}

/**
 * Aggregates sparse updates from any source together in the model
 * In this case the updates are gradients aggregated in the graph
 * @tparam TensorType
 * @param gradient
 * @param updated_rows
 */
template <class TensorType>
void ClientAlgorithm<TensorType>::AggregateSparseUpdate(VectorTensorType const &gradients,
                                                        VectorSizeVector const &updated_rows)
{
  auto grad_it = gradients.begin();
  auto rows_it = updated_rows.begin();

  for (auto &trainable : graph_ptr_->GetTrainables())
  {
    auto weights_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(trainable);
    weights_ptr->AddToGradient(*grad_it, *rows_it);
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
