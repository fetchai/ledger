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

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * @brief A loader for loading data in the "c2v" format from a file.
 * The "c2v" format looks like the following:
 * """
 * <functione_name_1> <start_word_1>,<path_1>,<terminating_word_1> <start_word_2>,<path_2>,<terminating_word_2> ... \n
 * <functione_name_2> ... 
 * """
 * The triple <start_word>,<path>,<terminating_word> is called context.
 * @tparam DataType the type of "feature" vector, the context triple (hashed)
 * @tparam LabelType the type of the "target", i.e. a (hashed) function name
 */
template <typename DataType, typename LabelType>
class C2VLoader : public DataLoader<DataType, LabelType>
{
public:
  using SizeType = typename DataType::value_type;

  using WordIdxType   = std::vector<std::vector<SizeType>>;
  using VocabType     = std::unordered_map<std::string, SizeType>;
  using SentencesType = std::vector<std::vector<std::string>>;
  using umap_str_int  = std::unordered_map<std::string, SizeType>;
  using umap_int_str  = std::unordered_map<SizeType, std::string>;

  C2VLoader()
    : data_iterator_position(0){};

  /**
   * @brief Gets the next pair of feature and target
   * 
   * @return std::pair<DataType, LabelType> 
   */
  std::pair<DataType, LabelType> GetNext();

  /**
   * @brief Gets the number of feature/target pairs
   * 
   * @return std::uint64_t 
   */
  std::uint64_t Size() const;

  /**
   * @brief Indicates if the GetNext() generater is at its end
   * 
   * @return true 
   * @return false 
   */
  bool IsDone() const;

  /**
   * @brief Resets the GetNext() generator
   * 
   */
  void Reset();


/**
 * @brief Adds (raw) data in the c2v format and creates the lookup maps
 * 
 * @param text 
 */
  void                            AddData(std::string const &text);

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

private:
  umap_str_int function_name_counter;
  umap_str_int path_counter;
  umap_str_int word_counter;

  umap_str_int function_name_to_idx;
  umap_str_int path_to_idx;
  umap_str_int word_to_idx;

  umap_int_str idx_to_function_name;
  umap_int_str idx_to_path;
  umap_int_str idx_to_word;

  std::vector<std::pair<DataType, LabelType>> data;
  SizeType                                    data_iterator_position;

/**
 * @brief Creates an unordered map for hashing strings from a counter (unordered map counting the occurences of words in the input)
 * 
 * @param counter unordered map with counts of words
 * @param name_to_idx unordered map for storing the mapping string->numeric
 * @param idx_to_name unordered map for storing the mapping numeric->string
 */
  static void createIdxUMapsFromCounter(umap_str_int &counter, umap_str_int &name_to_idx,
                                        umap_int_str &idx_to_name);

/**
 * @brief Add value to the unordered map for counting strings. If the provided string exists, the counter is set +1, otherwise
 * its set to 1
 * 
 * @param umap the unordered map where the counts are stored
 * @param word the string to be counted.
 */
  static void                     addValueToCounter(umap_str_int &umap, std::string word);
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

      std::pair<DataType, LabelType> input_data_pair(
          std::vector<SizeType>{source_word_idx, path_idx, target_word_idx}, function_name_idx);

      this->data.push_back(input_data_pair);
    }
  }
}

template <typename DataType, typename LabelType>
std::pair<DataType, LabelType> C2VLoader<DataType, LabelType>::GetNext()
{
  auto return_value = this->data[this->data_iterator_position];
  this->data_iterator_position += 1;
  return this->data[this->data_iterator_position];
};

template <typename DataType, typename LabelType>
std::uint64_t C2VLoader<DataType, LabelType>::Size() const
{
  return this->data.size();
};

template <typename DataType, typename LabelType>
bool C2VLoader<DataType, LabelType>::IsDone() const
{
  return ((this->data).size() == (this->data_iterator_position));
};

template <typename DataType, typename LabelType>
void C2VLoader<DataType, LabelType>::Reset()
{
  this->data_iterator_position = SizeType{0};
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
std::vector<std::string> C2VLoader<DataType, LabelType>::splitStringByChar(
    std::stringstream input, const char *sep)
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
typename C2VLoader<DataType, LabelType>::SizeType
C2VLoader<DataType, LabelType>::addToIdxUMaps(
    std::string input, typename C2VLoader<DataType, LabelType>::umap_str_int &name_to_idx,
    typename C2VLoader<DataType, LabelType>::umap_int_str &idx_to_name)
{
  if (name_to_idx.find(input) == name_to_idx.end())
  {
    name_to_idx[input]              = name_to_idx.size();
    idx_to_name[name_to_idx.size()] = input;
    return name_to_idx.size();
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