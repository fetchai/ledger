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

#include "dmlf/distributed_learning/client_params.hpp"
#include "dmlf/distributed_learning/distributed_learning_client.hpp"
#include "json/document.hpp"
#include "math/base_types.hpp"

namespace fetch {
namespace dmlf {
namespace distributed_learning {
namespace utilities {

/**
 * Averages weights between all clients
 * @param clients
 */
template <typename TensorType>
void SynchroniseWeights(
    std::vector<std::shared_ptr<fetch::dmlf::distributed_learning::TrainingClient<TensorType>>>
        clients)
{
  std::vector<TensorType> new_weights = clients[0]->GetWeights();

  // Sum all weights
  for (typename TensorType::SizeType i{1}; i < clients.size(); ++i)
  {
    std::vector<TensorType> other_weights = clients[i]->GetWeights();

    for (typename TensorType::SizeType j{0}; j < other_weights.size(); j++)
    {
      fetch::math::Add(new_weights.at(j), other_weights.at(j), new_weights.at(j));
    }
  }

  // Divide weights by number of clients to calculate the average
  for (typename TensorType::SizeType j{0}; j < new_weights.size(); j++)
  {
    fetch::math::Divide(new_weights.at(j), static_cast<typename TensorType::Type>(clients.size()),
                        new_weights.at(j));
  }

  // Update models of all clients by average model
  for (auto &c : clients)
  {
    c->SetWeights(new_weights);
  }
}

/**
 * Get loss of given model on given dataset
 * @param g_ptr model
 * @param data_tensor input
 * @param label_tensor label
 * @return
 */
template <typename TensorType>
typename TensorType::Type Test(std::shared_ptr<fetch::ml::Graph<TensorType>> const &g_ptr,
                               TensorType const &data_tensor, TensorType const &label_tensor)
{
  g_ptr->SetInput("Input", data_tensor);
  g_ptr->SetInput("Label", label_tensor);
  return *(g_ptr->Evaluate("Error").begin());
}

/**
 * Split data to multiple parts
 * @param data input data
 * @param number_of_parts number of parts
 * @return vector of tensors
 */
template <typename TensorType>
std::vector<TensorType> Split(TensorType &data, typename TensorType::SizeType number_of_parts)
{
  using SizeType = typename TensorType::SizeType;

  SizeType axis      = data.shape().size() - 1;
  SizeType data_size = data.shape().at(axis);

  // Split data for each client
  std::vector<SizeType> splitting_points;

  SizeType client_data_size         = data_size / number_of_parts;
  SizeType index                    = 0;
  SizeType current_client_data_size = client_data_size;
  for (SizeType i = 0; i < number_of_parts; i++)
  {
    if (i == number_of_parts - 1)
    {
      current_client_data_size = data_size - index;
    }
    splitting_points.push_back(current_client_data_size);
    index += client_data_size;
  }

  return TensorType::Split(data, splitting_points, axis);
}

template <typename TensorType>
void Shuffle(TensorType &data, TensorType &labels, typename TensorType::SizeType const &seed = 54)
{
  using SizeType = typename TensorType::SizeType;

  TensorType data_out   = data.Copy();
  TensorType labels_out = labels.Copy();

  std::vector<SizeType> indices;
  SizeType              axis = data.shape().size() - 1;

  for (SizeType i{0}; i < data.shape().at(axis); i++)
  {
    indices.push_back(i);
  }

  fetch::random::LaggedFibonacciGenerator<> lfg(seed);
  fetch::random::Shuffle(lfg, indices, indices);

  for (SizeType i{0}; i < data.shape().at(axis); i++)
  {
    auto data_it       = data.View(i).begin();
    auto data_out_it   = data_out.View(indices.at(i)).begin();
    auto labels_it     = labels.View(i).begin();
    auto labels_out_it = labels_out.View(indices.at(i)).begin();

    while (data_it.is_valid())
    {
      *data_out_it = *data_it;

      ++data_it;
      ++data_out_it;
    }

    while (labels_it.is_valid())
    {
      *labels_out_it = *labels_it;

      ++labels_it;
      ++labels_out_it;
    }
  }

  data   = data_out;
  labels = labels_out;
}

template <typename TensorType>
fetch::dmlf::distributed_learning::ClientParams<typename TensorType::Type> ClientParamsFromJson(
    std::string const &fname, fetch::json::JSONDocument &doc)
{
  using DataType = typename TensorType::Type;
  using SizeType = fetch::math::SizeType;

  std::ifstream config_file(fname);
  std::string text((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
  doc.Parse(text.c_str());

  auto client_params = fetch::dmlf::distributed_learning::ClientParams<DataType>();

  // try to extract each parameter
  // none are mandatory so it's fine if they're missing

  if (!doc["batch_size"].IsUndefined())
  {
    client_params.batch_size = doc["batch_size"].As<SizeType>();
  }
  if (!doc["max_updates"].IsUndefined())
  {
    client_params.max_updates = doc["max_updates"].As<SizeType>();
  }
  if (!doc["learning_rate"].IsUndefined())
  {
    client_params.learning_rate = static_cast<DataType>(doc["learning_rate"].As<float>());
  }
  if (!(doc["print_loss"].IsUndefined()))
  {
    client_params.print_loss = doc["print_loss"].As<bool>();
  }
  if (!doc["inputs_names"].IsUndefined())
  {
    auto const &inputs_names   = doc["inputs_names"];
    client_params.inputs_names = std::vector<std::string>(inputs_names.size());
    for (std::size_t i = 0; i < inputs_names.size(); ++i)
    {
      client_params.inputs_names.at(i) = inputs_names[i].As<std::string>();
    }
  }
  if (!doc["label_name"].IsUndefined())
  {
    client_params.label_name = doc["label_name"].As<std::string>();
  }
  if (!doc["error_name"].IsUndefined())
  {
    client_params.error_name = doc["error_name"].As<std::string>();
  }

  return client_params;
}

}  // namespace utilities
}  // namespace distributed_learning
}  // namespace dmlf
}  // namespace fetch
