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

#include "math/base_types.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define EMPTY_CONTEXT_STRING "EMPTY_CONTEXT_STRING"

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * @brief A loader for loading data in the "c2v" format from a file.
 * The "c2v" format looks like the following:
 * """
 * <functione_name_1> <start_word_1>,<path_1>,<terminating_word_1>
 * <start_word_2>,<path_2>,<terminating_word_2> ... \n <functione_name_2> ...
 * """
 * The triple <start_word>,<path>,<terminating_word> is called context.
 * @tparam InputType the type of "feature" vector, the context triple (hashed)
 * @tparam LabelType the type of the "target", i.e. a (hashed) function name
 */
template <typename LabelType, typename InputType>
class C2VLoader : public DataLoader<LabelType, InputType>
{
public:
  using TensorType              = InputType;
  using Type                    = typename TensorType::Type;
  using SizeType                = typename TensorType::SizeType;
  using ContextTuple            = std::tuple<SizeType, SizeType, SizeType>;
  using ContextVector           = std::vector<TensorType>;
  using ContextLabelPair        = std::pair<SizeType, ContextTuple>;
  using ContextTensorsLabelPair = std::pair<LabelType, ContextVector>;

  using WordIdxType   = std::vector<std::vector<SizeType>>;
  using VocabType     = std::unordered_map<std::string, SizeType>;
  using SentencesType = std::vector<std::vector<std::string>>;
  using umap_str_int  = std::unordered_map<std::string, SizeType>;
  using umap_int_str  = std::unordered_map<SizeType, std::string>;

  explicit C2VLoader(SizeType max_contexts_)
    : DataLoader<LabelType, InputType>()
    , iterator_position_get_next_context_(0)
    , iterator_position_get_next_(0)
    , current_function_index_(0)
    , max_contexts_(max_contexts_){};

  ContextLabelPair        GetNextContext();
  ContextTensorsLabelPair GetNext() override;

  SizeType Size() const override;
  bool     IsDone() const override;
  void     Reset() override;
  bool     AddData(InputType const &data, LabelType const &label) override;
  void     SetTestRatio(float new_test_ratio) override;
  void     SetValidationRatio(float new_validation_ratio) override;
  bool     IsModeAvailable(DataLoaderMode mode) override;

  void AddDataAsString(std::string const &c2v_input);
  void createIdxUMaps();

  umap_int_str umap_idx_to_functionname();
  umap_int_str umap_idx_to_path();
  umap_int_str umap_idx_to_word();

  umap_str_int umap_functionname_to_idx();
  umap_str_int umap_path_to_idx();
  umap_str_int umap_word_to_idx();

  umap_str_int function_name_counter();
  umap_str_int path_counter();
  umap_str_int word_counter();

private:
  std::vector<std::pair<SizeType, std::tuple<SizeType, SizeType, SizeType>>> data_;

  SizeType iterator_position_get_next_context_{fetch::math::numeric_max<SizeType>()};
  SizeType iterator_position_get_next_{fetch::math::numeric_max<SizeType>()};
  SizeType current_function_index_{fetch::math::numeric_max<SizeType>()};
  SizeType max_contexts_{fetch::math::numeric_max<SizeType>()};

  umap_str_int function_name_counter_;
  umap_str_int path_counter_;
  umap_str_int word_counter_;

  umap_str_int function_name_to_idx_;
  umap_str_int path_to_idx_;
  umap_str_int word_to_idx_;

  umap_int_str idx_to_function_name_;
  umap_int_str idx_to_path_;
  umap_int_str idx_to_word_;

  void UpdateCursor() override;

  static void createIdxUMapsFromCounter(umap_str_int &counter, umap_str_int &name_to_idx,
                                        umap_int_str &idx_to_name);

  static void addValueToCounter(umap_str_int &umap, std::string word);

  static std::vector<std::string> splitStringByChar(std::stringstream input, char const *sep);

  SizeType addToIdxUMaps(std::string const &input, umap_str_int &name_to_idx,
                         umap_int_str &idx_to_name);
};

template <typename LabelType, typename InputType>
bool C2VLoader<LabelType, InputType>::AddData(InputType const &data, LabelType const &label)
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
template <typename LabelType, typename InputType>
void C2VLoader<LabelType, InputType>::AddDataAsString(std::string const &c2v_input)
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
template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::ContextLabelPair
C2VLoader<LabelType, InputType>::GetNextContext()
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
template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::ContextTensorsLabelPair
C2VLoader<LabelType, InputType>::GetNext()
{
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
              static_cast<Type>(std::get<0>(data_[context_positions[i]].second));
          path_tensor(i, 0) = static_cast<Type>(std::get<1>(data_[context_positions[i]].second));
          target_word_tensor(i, 0) =
              static_cast<Type>(std::get<2>(data_[context_positions[i]].second));
        }
        for (SizeType i{context_positions.size()}; i < this->max_contexts_; i++)
        {
          source_word_tensor(i, 0) = static_cast<Type>(this->word_to_idx_[EMPTY_CONTEXT_STRING]);
          path_tensor(i, 0)        = static_cast<Type>(this->path_to_idx_[EMPTY_CONTEXT_STRING]);
          target_word_tensor(i, 0) = static_cast<Type>(this->word_to_idx_[EMPTY_CONTEXT_STRING]);
        }
      }
      else
      {
        for (SizeType i{0}; i < this->max_contexts_; i++)
        {
          source_word_tensor(i, 0) =
              static_cast<Type>(std::get<0>(data_[context_positions[i]].second));
          path_tensor(i, 0) = static_cast<Type>(std::get<1>(data_[context_positions[i]].second));
          target_word_tensor(i, 0) =
              static_cast<Type>(std::get<2>(data_[context_positions[i]].second));
        }
      }
      ContextVector context_tensor_tuple = {source_word_tensor, path_tensor, target_word_tensor};

      TensorType y_true_vec({function_name_counter().size() + 1, 1});
      y_true_vec.Fill(Type{0});
      // Preparing the y_true vector (one-hot-encoded)
      y_true_vec.Set(old_function_index, Type{0}, Type{1});

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
template <typename LabelType, typename InputType>
typename InputType::SizeType C2VLoader<LabelType, InputType>::Size() const
{
  return data_.size();
}

/**
 * @brief Indicates if the GetNextContext() generater is at its end
 *
 * @return true
 * @return false
 */
template <typename LabelType, typename InputType>
bool C2VLoader<LabelType, InputType>::IsDone() const
{
  return ((data_).size() == (this->iterator_position_get_next_context_));
}

/**
 * @brief Resets the GetNextContext() generator
 *
 */
template <typename LabelType, typename InputType>
void C2VLoader<LabelType, InputType>::Reset()
{
  this->iterator_position_get_next_context_ = SizeType{0};
}

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::SetTestRatio(float new_test_ratio)
{
  FETCH_UNUSED(new_test_ratio);
  throw exceptions::InvalidMode("Test set splitting is not supported for this dataloader.");
}

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::SetValidationRatio(float new_validation_ratio)
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
template <typename LabelType, typename InputType>
void C2VLoader<LabelType, InputType>::addValueToCounter(
    typename C2VLoader<LabelType, InputType>::umap_str_int &umap, std::string word)
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
template <typename LabelType, typename InputType>
std::vector<std::string> C2VLoader<LabelType, InputType>::splitStringByChar(std::stringstream input,
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
template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::SizeType C2VLoader<LabelType, InputType>::addToIdxUMaps(
    std::string const &input, typename C2VLoader<LabelType, InputType>::umap_str_int &name_to_idx,
    typename C2VLoader<LabelType, InputType>::umap_int_str &idx_to_name)
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
template <typename LabelType, typename InputType>
void C2VLoader<LabelType, InputType>::createIdxUMapsFromCounter(
    typename C2VLoader<LabelType, InputType>::umap_str_int &counter,
    typename C2VLoader<LabelType, InputType>::umap_str_int &name_to_idx,
    typename C2VLoader<LabelType, InputType>::umap_int_str &idx_to_name)
{
  int idx = 0;
  for (auto kv : counter)
  {
    name_to_idx[kv.first] = idx;
    idx_to_name[idx]      = kv.first;
    idx++;
  }
}

/**
 * @brief Creates umaps mapping strings to indices from the counter
 */
template <typename LabelType, typename InputType>
void C2VLoader<LabelType, InputType>::createIdxUMaps()
{
  createIdxUMapsFromCounter(function_name_counter_, function_name_to_idx_, idx_to_function_name_);
  createIdxUMapsFromCounter(path_counter_, path_to_idx_, idx_to_path_);
  createIdxUMapsFromCounter(word_counter_, word_to_idx_, idx_to_word_);
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_int_str
C2VLoader<LabelType, InputType>::umap_idx_to_functionname()
{
  return this->idx_to_function_name_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_int_str
C2VLoader<LabelType, InputType>::umap_idx_to_path()
{
  return this->idx_to_path_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_int_str
C2VLoader<LabelType, InputType>::umap_idx_to_word()
{
  return this->idx_to_word_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_str_int
C2VLoader<LabelType, InputType>::umap_functionname_to_idx()
{
  return this->function_name_to_idx_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_str_int
C2VLoader<LabelType, InputType>::umap_path_to_idx()
{
  return this->path_to_idx_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_str_int
C2VLoader<LabelType, InputType>::umap_word_to_idx()
{
  return this->word_to_idx_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_str_int
C2VLoader<LabelType, InputType>::function_name_counter()
{
  return this->function_name_counter_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_str_int
C2VLoader<LabelType, InputType>::path_counter()
{
  return this->path_counter_;
}

template <typename LabelType, typename InputType>
typename C2VLoader<LabelType, InputType>::umap_str_int
C2VLoader<LabelType, InputType>::word_counter()
{
  return this->word_counter_;
}

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::UpdateCursor()
{
  if (this->mode_ != DataLoaderMode::TRAIN)
  {
    throw exceptions::InvalidMode("Other mode than training not supported yet.");
  }
}

template <typename LabelType, typename DataType>
bool C2VLoader<LabelType, DataType>::IsModeAvailable(DataLoaderMode mode)
{
  return mode == DataLoaderMode::TRAIN;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
