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
 * @tparam DataType the type of "feature" vector, the context triple (hashed)
 * @tparam LabelType the type of the "target", i.e. a (hashed) function name
 */
template <typename LabelType, typename DataType>
class C2VLoader : public DataLoader<LabelType, DataType>
{
public:
  using ArrayType               = DataType;
  using Type                    = typename ArrayType::Type;
  using SizeType                = typename ArrayType::SizeType;
  using ContextTuple            = typename std::tuple<SizeType, SizeType, SizeType>;
  using ContextTensorTuple      = typename std::vector<ArrayType>;
  using ContextLabelPair        = typename std::pair<LabelType, ContextTuple>;
  using ContextTensorsLabelPair = typename std::pair<LabelType, ContextTensorTuple>;

  using WordIdxType   = std::vector<std::vector<SizeType>>;
  using VocabType     = std::unordered_map<std::string, SizeType>;
  using SentencesType = std::vector<std::vector<std::string>>;
  using umap_str_int  = std::unordered_map<std::string, SizeType>;
  using umap_int_str  = std::unordered_map<SizeType, std::string>;

  C2VLoader(SizeType max_contexts_)
    : iterator_position_get_next_context_(0)
    , iterator_position_get_next_(0)
    , current_function_index_(0)
    , max_contexts_(max_contexts_){};

  /**
   * @brief Gets the next pair of feature and target
   *
   * @return std::pair<DataType, LabelType>
   */
  ContextLabelPair GetNextContext();

  /**
   * @brief Gets the next pair of feature (tensor triple) and target
   *
   * @return std::pair<std::tuple<ArrayType, ArrayType, ArrayType>, LabelType>;
   */
  ContextTensorsLabelPair GetNext() override;

  /**
   * @brief Gets the number of feature/target pairs
   *
   * @return std::uint64_t
   */
  std::uint64_t Size() const override;

  /**
   * @brief Indicates if the GetNextContext() generater is at its end
   *
   * @return true
   * @return false
   */
  bool IsDone() const override;

  /**
   * @brief Resets the GetNextContext() generator
   *
   */
  void Reset() override;

  /**
   * @brief Adds (raw) data in the c2v format and creates the lookup maps
   *
   * @param text The input corpus in the "correct" c2v format
   */
  void AddData(std::string const &text);

  /**
   * @brief Creates umaps mapping strings to indices from the counter
   */
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
  std::vector<std::pair<LabelType, std::tuple<SizeType, SizeType, SizeType>>> data;

private:
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

  /**
   * @brief Creates an unordered map for hashing strings from a counter (unordered map counting the
   * occurences of words in the input)
   *
   * @param counter unordered map with counts of words
   * @param name_to_idx unordered map for storing the mapping string->numeric
   * @param idx_to_name unordered map for storing the mapping numeric->string
   */
  static void createIdxUMapsFromCounter(umap_str_int &counter, umap_str_int &name_to_idx,
                                        umap_int_str &idx_to_name);

  /**
   * @brief Add value to the unordered map for counting strings. If the provided string exists, the
   * counter is set +1, otherwise its set to 1
   *
   * @param umap the unordered map where the counts are stored
   * @param word the string to be counted.
   */
  static void addValueToCounter(umap_str_int &umap, std::string word);
  /**
   * @brief method splitting a string(stream) by a separator character
   *
   * @param input the stringstream which should be splitted
   * @param sep the seprator character
   * @return std::vector<std::string> A vector of substrings
   */
  static std::vector<std::string> splitStringByChar(std::stringstream input, const char *sep);

  /**
   * @brief Adds a string to the unordered maps for hashing and outputs the hashing index
   *
   * @param input input string
   * @param name_to_idx unordered map for mapping string->numeric
   * @param idx_to_name unordered map for mapping numeric->string
   * @return SizeType index of the string in the unordered maps
   */
  SizeType addToIdxUMaps(std::string const &input, umap_str_int &name_to_idx,
                         umap_int_str &idx_to_name);
};

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::AddData(std::string const &c2v_input)
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
    const char *      sep = ",";

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

      std::pair<LabelType, std::tuple<SizeType, SizeType, SizeType>> input_data_pair(
          function_name_idx, std::make_tuple(source_word_idx, path_idx, target_word_idx));

      this->data.push_back(input_data_pair);
    }
  }
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::ContextLabelPair
C2VLoader<LabelType, DataType>::GetNextContext()
{
  auto return_value = this->data[this->iterator_position_get_next_context_];
  this->iterator_position_get_next_context_ += 1;
  return return_value;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::ContextTensorsLabelPair
C2VLoader<LabelType, DataType>::GetNext()
{
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

      ArrayType source_word_tensor({this->max_contexts_});
      ArrayType path_tensor({this->max_contexts_});
      ArrayType target_word_tensor({this->max_contexts_});

      if (context_positions.size() <= this->max_contexts_)
      {
        for (uint64_t i{0}; i < context_positions.size(); i++)
        {
          source_word_tensor.Set(
              i, static_cast<Type>(std::get<0>(this->data[context_positions[i]].second)));
          path_tensor.Set(i,
                          static_cast<Type>(std::get<1>(this->data[context_positions[i]].second)));
          target_word_tensor.Set(
              i, static_cast<Type>(std::get<2>(this->data[context_positions[i]].second)));
        }
        for (uint64_t i{context_positions.size()}; i < this->max_contexts_; i++)
        {
          source_word_tensor.Set(i, static_cast<Type>(this->word_to_idx_[EMPTY_CONTEXT_STRING]));
          path_tensor.Set(i, static_cast<Type>(this->path_to_idx_[EMPTY_CONTEXT_STRING]));
          target_word_tensor.Set(i, static_cast<Type>(this->word_to_idx_[EMPTY_CONTEXT_STRING]));
        }
      }
      else
      {
        for (uint64_t i{0}; i < this->max_contexts_; i++)
        {
          source_word_tensor.Set(
              i, static_cast<Type>(std::get<0>(this->data[context_positions[i]].second)));
          path_tensor.Set(i,
                          static_cast<Type>(std::get<1>(this->data[context_positions[i]].second)));
          target_word_tensor.Set(
              i, static_cast<Type>(std::get<2>(this->data[context_positions[i]].second)));
        }
      }
      ContextTensorTuple context_tensor_tuple = {source_word_tensor, path_tensor,
                                                 target_word_tensor};

      ContextTensorsLabelPair return_pair{old_function_index, context_tensor_tuple};

      return return_pair;
    }
    iteration_start = false;
  }
}

template <typename LabelType, typename DataType>
std::uint64_t C2VLoader<LabelType, DataType>::Size() const
{
  return this->data.size();
}

template <typename LabelType, typename DataType>
bool C2VLoader<LabelType, DataType>::IsDone() const
{
  return ((this->data).size() == (this->iterator_position_get_next_context_));
}

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::Reset()
{
  this->iterator_position_get_next_context_ = SizeType{0};
}

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::addValueToCounter(
    typename C2VLoader<LabelType, DataType>::umap_str_int &umap, std::string word)
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

template <typename LabelType, typename DataType>
std::vector<std::string> C2VLoader<LabelType, DataType>::splitStringByChar(std::stringstream input,
                                                                           const char *      sep)
{
  std::vector<std::string> splitted_string;
  std::string              segment;
  while (std::getline(input, segment, *sep))
  {
    splitted_string.push_back(segment);
  }
  return splitted_string;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::SizeType C2VLoader<LabelType, DataType>::addToIdxUMaps(
    std::string const &input, typename C2VLoader<LabelType, DataType>::umap_str_int &name_to_idx,
    typename C2VLoader<LabelType, DataType>::umap_int_str &idx_to_name)
{
  if (name_to_idx.find(input) == name_to_idx.end())
  {
    auto index_of_new_word{name_to_idx.size()};
    std::cout << "Not found, added " << input << " to " << index_of_new_word << std::endl;
    name_to_idx[input]             = index_of_new_word;
    idx_to_name[index_of_new_word] = input;
    return index_of_new_word;
  }
  else
  {
    return name_to_idx[input];
  }
}

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::createIdxUMapsFromCounter(
    typename C2VLoader<LabelType, DataType>::umap_str_int &counter,
    typename C2VLoader<LabelType, DataType>::umap_str_int &name_to_idx,
    typename C2VLoader<LabelType, DataType>::umap_int_str &idx_to_name)
{
  int idx = 0;
  for (auto kv : counter)
  {
    name_to_idx[kv.first] = idx;
    idx_to_name[idx]      = kv.first;
    idx++;
  }
}

template <typename LabelType, typename DataType>
void C2VLoader<LabelType, DataType>::createIdxUMaps()
{
  createIdxUMapsFromCounter(function_name_counter_, function_name_to_idx_, idx_to_function_name_);
  createIdxUMapsFromCounter(path_counter_, path_to_idx_, idx_to_path_);
  createIdxUMapsFromCounter(word_counter_, word_to_idx_, idx_to_word_);
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_int_str
C2VLoader<LabelType, DataType>::umap_idx_to_functionname()
{
  return this->idx_to_function_name_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_int_str
C2VLoader<LabelType, DataType>::umap_idx_to_path()
{
  return this->idx_to_path_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_int_str
C2VLoader<LabelType, DataType>::umap_idx_to_word()
{
  return this->idx_to_word_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_str_int
C2VLoader<LabelType, DataType>::umap_functionname_to_idx()
{
  return this->function_name_to_idx_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_str_int
C2VLoader<LabelType, DataType>::umap_path_to_idx()
{
  return this->path_to_idx_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_str_int
C2VLoader<LabelType, DataType>::umap_word_to_idx()
{
  return this->word_to_idx_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_str_int
C2VLoader<LabelType, DataType>::function_name_counter()
{
  return this->function_name_counter_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_str_int C2VLoader<LabelType, DataType>::path_counter()
{
  return this->path_counter_;
}

template <typename LabelType, typename DataType>
typename C2VLoader<LabelType, DataType>::umap_str_int C2VLoader<LabelType, DataType>::word_counter()
{
  return this->word_counter_;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch