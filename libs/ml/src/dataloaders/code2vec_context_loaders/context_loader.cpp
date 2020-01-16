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

#include "ml/dataloaders/code2vec_context_loaders/context_loader.hpp"

#include "core/macros.hpp"
#include "math/base_types.hpp"
#include "math/tensor/tensor.hpp"

#include "ml/dataloaders/dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename TensorType>
bool C2VLoader<TensorType>::AddData(std::vector<TensorType> const &data, TensorType const &label)
{
  FETCH_UNUSED(data);
  FETCH_UNUSED(label);
  throw exceptions::InvalidMode(
      "AddData not implemented for Code2Vec example. use AddDataAsString");
}

/**
 * @brief Adds (raw) data in the c2v format and creates the lookup maps
 *
 * @param text The input corpus in the "correct" c2v format
 */
template <typename TensorType>
void C2VLoader<TensorType>::AddDataAsString(std::string const &c2v_input)
{

  std::stringstream c2v_input_ss(c2v_input);
  std::string       c2v_input_line;
  std::string       function_name;
  std::string       context;

  addToIdxUMaps(EMPTY_CONTEXT_STRING, function_name_to_idx_, idx_to_function_name_);
  addToIdxUMaps(EMPTY_CONTEXT_STRING, word_to_idx_, idx_to_word_);
  addToIdxUMaps(EMPTY_CONTEXT_STRING, path_to_idx_, idx_to_path_);

  while (std::getline(c2v_input_ss, c2v_input_line, '\n'))
  {
    std::stringstream c2v_input_line_ss(c2v_input_line);
    char const *      sep = ",";

    c2v_input_line_ss >> function_name;

    addValueToCounter(function_name_counter_, function_name);
    SizeType function_name_idx =
        addToIdxUMaps(function_name, function_name_to_idx_, idx_to_function_name_);

    while (c2v_input_line_ss >> context)
    {
      std::vector<std::string> context_string_splitted =
          splitStringByChar(std::stringstream(context), sep);

      addValueToCounter(word_counter_, context_string_splitted[0]);
      addValueToCounter(path_counter_, context_string_splitted[1]);
      addValueToCounter(word_counter_, context_string_splitted[2]);

      SizeType source_word_idx =
          addToIdxUMaps(context_string_splitted[0], word_to_idx_, idx_to_word_);
      SizeType path_idx = addToIdxUMaps(context_string_splitted[1], path_to_idx_, idx_to_path_);
      SizeType target_word_idx =
          addToIdxUMaps(context_string_splitted[2], word_to_idx_, idx_to_word_);

      std::pair<SizeType, std::tuple<SizeType, SizeType, SizeType>> input_data_pair(
          function_name_idx, std::make_tuple(source_word_idx, path_idx, target_word_idx));

      data_.push_back(input_data_pair);
    }
  }
}

/**
 * @brief Gets the next pair of feature and target
 *
 * @return std::pair<InputType, LabelType>
 */
template <typename TensorType>
typename C2VLoader<TensorType>::ContextLabelPair C2VLoader<TensorType>::GetNextContext()
{
  auto return_value = data_[this->iterator_position_get_next_context_];
  this->iterator_position_get_next_context_ += 1;
  return return_value;
}

/**
 * @brief Gets the next pair of feature (vector of 3 tensors) and target
 *
 * @return std::pair<std::vector<TensorType>, LabelType>;
 */
template <typename TensorType>
typename C2VLoader<TensorType>::ContextTensorsLabelPair C2VLoader<TensorType>::GetNext()
{
  using DataType = typename TensorType::Type;

  if (this->random_mode_)
  {
    throw exceptions::InvalidMode("Random sampling not implemented for C2VLoader");
  }

  std::vector<SizeType> context_positions;
  SizeType              old_function_index{0};
  bool                  iteration_start = true;

  while (true)
  {
    auto             current_context_position = this->iterator_position_get_next_context_;
    ContextLabelPair input                    = this->GetNextContext();
    if ((iteration_start || (input.first == old_function_index)) && !this->IsDone())
    {
      old_function_index = input.first;
      context_positions.push_back(current_context_position);
    }
    else
    {
      if (!this->IsDone())
      {
        this->iterator_position_get_next_context_--;
      }
      else
      {
        context_positions.push_back(current_context_position);
      }

      TensorType source_word_tensor({this->max_contexts_, 1});
      TensorType path_tensor({this->max_contexts_, 1});
      TensorType target_word_tensor({this->max_contexts_, 1});

      if (context_positions.size() <= this->max_contexts_)
      {
        for (SizeType i{0}; i < context_positions.size(); i++)
        {

          source_word_tensor(i, 0) =
              static_cast<DataType>(std::get<0>(data_[context_positions[i]].second));
          path_tensor(i, 0) =
              static_cast<DataType>(std::get<1>(data_[context_positions[i]].second));
          target_word_tensor(i, 0) =
              static_cast<DataType>(std::get<2>(data_[context_positions[i]].second));
        }
        for (SizeType i{context_positions.size()}; i < this->max_contexts_; i++)
        {
          source_word_tensor(i, 0) =
              static_cast<DataType>(this->word_to_idx_[EMPTY_CONTEXT_STRING]);
          path_tensor(i, 0) = static_cast<DataType>(this->path_to_idx_[EMPTY_CONTEXT_STRING]);
          target_word_tensor(i, 0) =
              static_cast<DataType>(this->word_to_idx_[EMPTY_CONTEXT_STRING]);
        }
      }
      else
      {
        for (SizeType i{0}; i < this->max_contexts_; i++)
        {
          source_word_tensor(i, 0) =
              static_cast<DataType>(std::get<0>(data_[context_positions[i]].second));
          path_tensor(i, 0) =
              static_cast<DataType>(std::get<1>(data_[context_positions[i]].second));
          target_word_tensor(i, 0) =
              static_cast<DataType>(std::get<2>(data_[context_positions[i]].second));
        }
      }
      ContextVector context_tensor_tuple = {source_word_tensor, path_tensor, target_word_tensor};

      TensorType y_true_vec({function_name_counter().size() + 1, 1});
      y_true_vec.Fill(DataType{0});
      // Preparing the y_true vector (one-hot-encoded)
      y_true_vec.Set(old_function_index, DataType{0}, DataType{1});

      ContextTensorsLabelPair return_pair{y_true_vec, context_tensor_tuple};

      return return_pair;
    }
    iteration_start = false;
  }
}

/**
 * @brief Gets the number of feature/target pairs
 *
 * @return uint64_t
 */
template <typename TensorType>
fetch::math::SizeType C2VLoader<TensorType>::Size() const
{
  return data_.size();
}

/**
 * @brief Indicates if the GetNextContext() generater is at its end
 *
 * @return true
 * @return false
 */
template <typename TensorType>
bool C2VLoader<TensorType>::IsDone() const
{
  return ((data_).size() == (this->iterator_position_get_next_context_));
}

/**
 * @brief Resets the GetNextContext() generator
 *
 */
template <typename TensorType>
void C2VLoader<TensorType>::Reset()
{
  this->iterator_position_get_next_context_ = SizeType{0};
}

template <typename TypeParam>
void C2VLoader<TypeParam>::SetTestRatio(fixed_point::fp32_t new_test_ratio)
{
  FETCH_UNUSED(new_test_ratio);
  throw exceptions::InvalidMode("Test set splitting is not supported for this dataloader.");
}

template <typename TypeParam>
void C2VLoader<TypeParam>::SetValidationRatio(fixed_point::fp32_t new_validation_ratio)
{
  FETCH_UNUSED(new_validation_ratio);
  throw exceptions::InvalidMode("Validation set splitting is not supported for this dataloader.");
}

/**
 * @brief Add value to the unordered map for counting strings. If the provided string exists, the
 * counter is set +1, otherwise its set to 1
 *
 * @param umap the unordered map where the counts are stored
 * @param word the string to be counted.
 */
template <typename TensorType>
void C2VLoader<TensorType>::addValueToCounter(typename C2VLoader<TensorType>::umap_str_int &umap,
                                              const std::string &                           word)
{
  if (umap.find(word) == umap.end())
  {
    umap[word] = 1;
  }
  else
  {
    umap[word] += 1;
  }
}

/**
 * @brief method splitting a string(stream) by a separator character
 *
 * @param input the stringstream which should be split
 * @param sep the separator character
 * @return std::vector<std::string> A vector of substrings
 */
template <typename TensorType>
std::vector<std::string> C2VLoader<TensorType>::splitStringByChar(std::stringstream input,
                                                                  char const *      sep)
{
  std::vector<std::string> split_string;
  std::string              segment;
  while (std::getline(input, segment, *sep))
  {
    split_string.push_back(segment);
  }
  return split_string;
}

/**
 * @brief Adds a string to the unordered maps for hashing and outputs the hashing index
 *
 * @param input input string
 * @param name_to_idx unordered map for mapping string->numeric
 * @param idx_to_name unordered map for mapping numeric->string
 * @return SizeType index of the string in the unordered maps
 */
template <typename TensorType>
typename C2VLoader<TensorType>::SizeType C2VLoader<TensorType>::addToIdxUMaps(
    std::string const &input, typename C2VLoader<TensorType>::umap_str_int &name_to_idx,
    typename C2VLoader<TensorType>::umap_int_str &idx_to_name)
{
  if (name_to_idx.find(input) == name_to_idx.end())
  {
    auto index_of_new_word{name_to_idx.size()};
    std::cout << "Not found, added " << input << " to " << index_of_new_word << std::endl;
    name_to_idx[input]             = index_of_new_word;
    idx_to_name[index_of_new_word] = input;
    return index_of_new_word;
  }

  return name_to_idx[input];
}

/**
 * @brief Creates an unordered map for hashing strings from a counter (unordered map counting the
 * occurrences of words in the input)
 *
 * @param counter unordered map with counts of words
 * @param name_to_idx unordered map for storing the mapping string->numeric
 * @param idx_to_name unordered map for storing the mapping numeric->string
 */
template <typename TensorType>
void C2VLoader<TensorType>::createIdxUMapsFromCounter(
    typename C2VLoader<TensorType>::umap_str_int &counter,
    typename C2VLoader<TensorType>::umap_str_int &name_to_idx,
    typename C2VLoader<TensorType>::umap_int_str &idx_to_name)
{
  int idx = 0;
  for (auto kv : counter)
  {
    name_to_idx[kv.first]      = uint64_t(idx);
    idx_to_name[uint64_t(idx)] = kv.first;
    idx++;
  }
}

/**
 * @brief Creates umaps mapping strings to indices from the counter
 */
template <typename TensorType>
void C2VLoader<TensorType>::createIdxUMaps()
{
  createIdxUMapsFromCounter(function_name_counter_, function_name_to_idx_, idx_to_function_name_);
  createIdxUMapsFromCounter(path_counter_, path_to_idx_, idx_to_path_);
  createIdxUMapsFromCounter(word_counter_, word_to_idx_, idx_to_word_);
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_int_str C2VLoader<TensorType>::umap_idx_to_functionname()
{
  return this->idx_to_function_name_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_int_str C2VLoader<TensorType>::umap_idx_to_path()
{
  return this->idx_to_path_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_int_str C2VLoader<TensorType>::umap_idx_to_word()
{
  return this->idx_to_word_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_str_int C2VLoader<TensorType>::umap_functionname_to_idx()
{
  return this->function_name_to_idx_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_str_int C2VLoader<TensorType>::umap_path_to_idx()
{
  return this->path_to_idx_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_str_int C2VLoader<TensorType>::umap_word_to_idx()
{
  return this->word_to_idx_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_str_int C2VLoader<TensorType>::function_name_counter()
{
  return this->function_name_counter_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_str_int C2VLoader<TensorType>::path_counter()
{
  return this->path_counter_;
}

template <typename TensorType>
typename C2VLoader<TensorType>::umap_str_int C2VLoader<TensorType>::word_counter()
{
  return this->word_counter_;
}

template <typename TypeParam>
void C2VLoader<TypeParam>::UpdateCursor()
{
  if (this->mode_ != DataLoaderMode::TRAIN)
  {
    throw exceptions::InvalidMode("Other mode than training not supported yet.");
  }
}

template <typename TypeParam>
bool C2VLoader<TypeParam>::IsModeAvailable(DataLoaderMode mode)
{
  return mode == DataLoaderMode::TRAIN;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class C2VLoader<math::Tensor<std::int8_t>>;
template class C2VLoader<math::Tensor<std::int16_t>>;
template class C2VLoader<math::Tensor<std::int32_t>>;
template class C2VLoader<math::Tensor<std::int64_t>>;
template class C2VLoader<math::Tensor<float>>;
template class C2VLoader<math::Tensor<double>>;
template class C2VLoader<math::Tensor<fixed_point::fp32_t>>;
template class C2VLoader<math::Tensor<fixed_point::fp64_t>>;
template class C2VLoader<math::Tensor<fixed_point::fp128_t>>;

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
