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

#include "ml/dataloaders/dataloader.hpp"
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
template <typename DataType, typename LabelType>
class C2VLoader : public DataLoader<DataType, LabelType>
{
public:
  using ArrayType               = typename std::tuple_element<0, DataType>::type;
  using SizeType                = typename ArrayType::SizeType;
  using ContextTuple            = typename std::tuple<SizeType, SizeType, SizeType>;
  using ContextTensorTuple      = typename std::tuple<ArrayType, ArrayType, ArrayType>;
  using ContextLabelPair        = typename std::pair<ContextTuple, LabelType>;
  using ContextTensorsLabelPair = typename std::pair<ContextTensorTuple, LabelType>;

  using WordIdxType   = std::vector<std::vector<SizeType>>;
  using VocabType     = std::unordered_map<std::string, SizeType>;
  using SentencesType = std::vector<std::vector<std::string>>;
  using umap_str_int  = std::unordered_map<std::string, SizeType>;
  using umap_int_str  = std::unordered_map<SizeType, std::string>;

  C2VLoader(SizeType max_contexts)
    : iterator_position_get_next_context(0)
    , iterator_position_get_next(0)
    , current_function_index(0)
    , max_contexts(max_contexts){};

  /**
   * @brief Gets the next pair of feature and target
   *
   * @return std::pair<DataType, LabelType>
   */
  ContextLabelPair GetNextContext();

  ContextTensorsLabelPair GetNext();

  /**
   * @brief Gets the number of feature/target pairs
   *
   * @return std::uint64_t
   */
  std::uint64_t Size() const;

  /**
   * @brief Indicates if the GetNextContext() generater is at its end
   *
   * @return true
   * @return false
   */
  bool IsDone() const;

  /**
   * @brief Resets the GetNextContext() generator
   *
   */
  void Reset();

  /**
   * @brief Adds (raw) data in the c2v format and creates the lookup maps
   *
   * @param text
   */
  void AddData(std::string const &text);

  void createIdxUMaps();

  umap_int_str GetUmapIdxToFunctionName();
  umap_int_str GetUmapIdxToPath();
  umap_int_str GetUmapIdxToWord();

  umap_str_int GetUmapFunctioNameToIdx();
  umap_str_int GetUmapPathToIdx();
  umap_str_int GetUmapWordToIdx();

  umap_str_int GetCounterFunctionNames();
  umap_str_int GetCounterPaths();
  umap_str_int GetCounterWords();
  std::vector<std::pair<std::tuple<SizeType, SizeType, SizeType>, LabelType>> data;

private:
  SizeType max_contexts;

  umap_str_int function_name_counter;
  umap_str_int path_counter;
  umap_str_int word_counter;

  umap_str_int function_name_to_idx;
  umap_str_int path_to_idx;
  umap_str_int word_to_idx;

  umap_int_str idx_to_function_name;
  umap_int_str idx_to_path;
  umap_int_str idx_to_word;

  SizeType iterator_position_get_next_context;
  SizeType iterator_position_get_next;
  SizeType current_function_index;
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
  SizeType addToIdxUMaps(std::string input, umap_str_int &name_to_idx, umap_int_str &idx_to_name);
};

template <typename DataType, typename LabelType>
void C2VLoader<DataType, LabelType>::AddData(std::string const &c2v_input)
{

  std::stringstream c2v_input_ss(c2v_input);
  std::string       c2v_input_line;
  std::string       function_name;
  std::string       context;

  addToIdxUMaps(EMPTY_CONTEXT_STRING, function_name_to_idx, idx_to_function_name);
  addToIdxUMaps(EMPTY_CONTEXT_STRING, word_to_idx, idx_to_word);
  addToIdxUMaps(EMPTY_CONTEXT_STRING, path_to_idx, idx_to_path);

  while (std::getline(c2v_input_ss, c2v_input_line, '\n'))
  {
    std::stringstream c2v_input_line_ss(c2v_input_line);
    const char *      sep = ",";

    c2v_input_line_ss >> function_name;

    addValueToCounter(function_name_counter, function_name);
    SizeType function_name_idx =
        addToIdxUMaps(function_name, function_name_to_idx, idx_to_function_name);

    while (c2v_input_line_ss >> context)
    {
      std::vector<std::string> context_string_splitted =
          splitStringByChar(std::stringstream(context), sep);

      addValueToCounter(word_counter, context_string_splitted[0]);
      addValueToCounter(path_counter, context_string_splitted[1]);
      addValueToCounter(word_counter, context_string_splitted[2]);

      SizeType source_word_idx =
          addToIdxUMaps(context_string_splitted[0], word_to_idx, idx_to_word);
      SizeType path_idx = addToIdxUMaps(context_string_splitted[1], path_to_idx, idx_to_path);
      SizeType target_word_idx =
          addToIdxUMaps(context_string_splitted[2], word_to_idx, idx_to_word);

      std::pair<std::tuple<SizeType, SizeType, SizeType>, LabelType> input_data_pair(
          std::tuple<SizeType, SizeType, SizeType>{source_word_idx, path_idx, target_word_idx},
          function_name_idx);

      this->data.push_back(input_data_pair);
    }
  }
}

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::ContextLabelPair
C2VLoader<DataType, LabelType>::GetNextContext()
{
  auto return_value = this->data[this->iterator_position_get_next_context];
  this->iterator_position_get_next_context += 1;
  return return_value;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::ContextTensorsLabelPair
C2VLoader<DataType, LabelType>::GetNext()
{
  std::vector<SizeType> context_positions;
  SizeType              old_function_index{0};
  bool iteration_start = true;

  while (true)
  {
    auto current_context_position = this->iterator_position_get_next_context;
    ContextLabelPair input = this->GetNextContext();
    if ((iteration_start || (input.second == old_function_index)) && !this->IsDone())
    {
      old_function_index = input.second;
      context_positions.push_back(current_context_position);
    }
    else
    {
      if (!this->IsDone())
      {
        this->iterator_position_get_next_context--;
      }
      else{
        context_positions.push_back(current_context_position);
      }

      ArrayType source_word_tensor({this->max_contexts});
      ArrayType path_tensor({this->max_contexts});
      ArrayType target_word_tensor({this->max_contexts});

      if(context_positions.size() <= this->max_contexts){ 
        for(u_int64_t i{0}; i<context_positions.size(); i++){
              source_word_tensor.Set(i, std::get<0>(this->data[context_positions[i]].first));
              path_tensor.Set(i, std::get<1>(this->data[context_positions[i]].first));
              target_word_tensor.Set(i, std::get<2>(this->data[context_positions[i]].first));
        }
        for(u_int64_t i{context_positions.size()}; i<this->max_contexts; i++){
              source_word_tensor.Set(i, this->word_to_idx[EMPTY_CONTEXT_STRING]);
              path_tensor.Set(i, this->path_to_idx[EMPTY_CONTEXT_STRING]);
              target_word_tensor.Set(i, this->word_to_idx[EMPTY_CONTEXT_STRING]);
        }        
      } else{
        for(u_int64_t i{0}; i<this->max_contexts; i++){
              source_word_tensor.Set(i, std::get<0>(this->data[context_positions[i]].first));
              path_tensor.Set(i, std::get<1>(this->data[context_positions[i]].first));
              target_word_tensor.Set(i, std::get<2>(this->data[context_positions[i]].first));
        }
      }
      ContextTensorTuple context_tensor_tuple = std::make_tuple(source_word_tensor, path_tensor, target_word_tensor);

      ContextTensorsLabelPair return_pair{context_tensor_tuple, old_function_index};

      return return_pair;
    }
    iteration_start = false;
  }

};

template <typename DataType, typename LabelType>
std::uint64_t C2VLoader<DataType, LabelType>::Size() const
{
  return this->data.size();
};

template <typename DataType, typename LabelType>
bool C2VLoader<DataType, LabelType>::IsDone() const
{
  return ((this->data).size() == (this->iterator_position_get_next_context));
};

template <typename DataType, typename LabelType>
void C2VLoader<DataType, LabelType>::Reset()
{
  this->iterator_position_get_next_context = SizeType{0};
};

template <typename DataType, typename LabelType>
void C2VLoader<DataType, LabelType>::addValueToCounter(
    typename C2VLoader<DataType, LabelType>::umap_str_int &umap, std::string word)
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

template <typename DataType, typename LabelType>
std::vector<std::string> C2VLoader<DataType, LabelType>::splitStringByChar(std::stringstream input,
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

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::SizeType C2VLoader<DataType, LabelType>::addToIdxUMaps(
    std::string input, typename C2VLoader<DataType, LabelType>::umap_str_int &name_to_idx,
    typename C2VLoader<DataType, LabelType>::umap_int_str &idx_to_name)
{
  if (name_to_idx.find(input) == name_to_idx.end())
  {
    auto index_of_new_word{name_to_idx.size()};
    std::cout << "Not found, added " << input << " to " << index_of_new_word << std::endl;
    name_to_idx[input]              = index_of_new_word;
    idx_to_name[index_of_new_word] = input;
    return index_of_new_word;
  }
  else
  {
    return name_to_idx[input];
  }
}

template <typename DataType, typename LabelType>
void C2VLoader<DataType, LabelType>::createIdxUMapsFromCounter(
    typename C2VLoader<DataType, LabelType>::umap_str_int &counter,
    typename C2VLoader<DataType, LabelType>::umap_str_int &name_to_idx,
    typename C2VLoader<DataType, LabelType>::umap_int_str &idx_to_name)
{
  int idx = 0;
  for (auto kv : counter)
  {
    name_to_idx[kv.first] = idx;
    idx_to_name[idx]      = kv.first;
    idx++;
  }
}

template <typename DataType, typename LabelType>
void C2VLoader<DataType, LabelType>::createIdxUMaps()
{
  createIdxUMapsFromCounter(function_name_counter, function_name_to_idx, idx_to_function_name);
  createIdxUMapsFromCounter(path_counter, path_to_idx, idx_to_path);
  createIdxUMapsFromCounter(word_counter, word_to_idx, idx_to_word);
}

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_int_str
C2VLoader<DataType, LabelType>::GetUmapIdxToFunctionName()
{
  return this->idx_to_function_name;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_int_str
C2VLoader<DataType, LabelType>::GetUmapIdxToPath()
{
  return this->idx_to_path;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_int_str
C2VLoader<DataType, LabelType>::GetUmapIdxToWord()
{
  return this->idx_to_word;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_str_int
C2VLoader<DataType, LabelType>::GetUmapFunctioNameToIdx()
{
  return this->function_name_to_idx;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_str_int
C2VLoader<DataType, LabelType>::GetUmapPathToIdx()
{
  return this->path_to_idx;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_str_int
C2VLoader<DataType, LabelType>::GetUmapWordToIdx()
{
  return this->word_to_idx;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_str_int
C2VLoader<DataType, LabelType>::GetCounterFunctionNames()
{
  return this->function_name_counter;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_str_int
C2VLoader<DataType, LabelType>::GetCounterPaths()
{
  return this->path_counter;
};

template <typename DataType, typename LabelType>
typename C2VLoader<DataType, LabelType>::umap_str_int
C2VLoader<DataType, LabelType>::GetCounterWords()
{
  return this->word_counter;
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch