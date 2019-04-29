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

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * A basic text loader that handles parsing text strings into a vocabulary
 * @tparam T  tensor type
 */
template <typename T>
class ContextLoader
{
public:
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

  using WordIdxType   = std::vector<std::vector<SizeType>>;
  using VocabType     = std::unordered_map<std::string, SizeType>;
  using SentencesType = std::vector<std::vector<std::string>>;
  using umap_str_int = std::unordered_map<std::string, int>;
  using umap_int_str = std::unordered_map<int, std::string>;

  ContextLoader() = default;

  void        AddData(std::string const &text);
  static void addValueToUnorderedMap(umap_str_int &umap, std::string word);
  static std::vector<std::string> splitStringByChar(std::stringstream input, const char *sep);

  void        createIdxUMaps();

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


  static void createIdxUMapsFromCounter(umap_str_int &counter,
                                        umap_str_int &name_to_idx,
                                        umap_int_str &idx_to_name);

};

template <typename T>
void ContextLoader<T>::AddData(std::string const &c2v_input)
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

    addValueToUnorderedMap(function_name_counter, function_name);

    while (c2v_input_line_ss >> context)
    {
      std::vector<std::string> context_string_splitted =
          splitStringByChar(std::stringstream(context), sep);

      addValueToUnorderedMap(word_counter, context_string_splitted[0]);
      addValueToUnorderedMap(path_counter, context_string_splitted[1]);
      addValueToUnorderedMap(word_counter, context_string_splitted[2]);
    }
  }
}

template <typename T>
void ContextLoader<T>::addValueToUnorderedMap(typename ContextLoader<T>::umap_str_int &umap,
                                              std::string                           word)
{
  if (umap.find(word) == umap.end())
  {
    umap[word] = 1;
  }
  else
  {
    umap[word] = +1;
  }
}
template <typename T>
std::vector<std::string> ContextLoader<T>::splitStringByChar(std::stringstream input,
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

template <typename T>
void ContextLoader<T>::createIdxUMapsFromCounter(typename ContextLoader<T>::umap_str_int &counter,
                                                 typename ContextLoader<T>::umap_str_int &name_to_idx,
                                                 typename ContextLoader<T>::umap_int_str &idx_to_name)
{
  int idx = 0;
  for (auto kv : counter)
  {
    name_to_idx[kv.first] = idx;
    idx_to_name[idx]      = kv.first;
    idx++;
  }
}

template <typename T>
void ContextLoader<T>::createIdxUMaps()
{
  createIdxUMapsFromCounter(function_name_counter, function_name_to_idx, idx_to_function_name);
  createIdxUMapsFromCounter(path_counter, path_to_idx, idx_to_path);
  createIdxUMapsFromCounter(word_counter, word_to_idx, idx_to_word);
}

template <typename T>
typename ContextLoader<T>::umap_int_str ContextLoader<T>::GetUmapIdxToFunctionName(){
  return this->idx_to_function_name;
};

template <typename T>
typename ContextLoader<T>::umap_int_str ContextLoader<T>::GetUmapIdxToPath(){
  return this->idx_to_path;
};

template <typename T>
typename ContextLoader<T>::umap_int_str ContextLoader<T>::GetUmapIdxToWord(){
  return this->idx_to_word;
};




template <typename T>
typename ContextLoader<T>::umap_str_int ContextLoader<T>::GetUmapFunctioNameToIdx(){
  return this->function_name_to_idx;
};

template <typename T>
typename ContextLoader<T>::umap_str_int ContextLoader<T>::GetUmapPathToIdx(){
  return this->path_to_idx;
};

template <typename T>
typename ContextLoader<T>::umap_str_int ContextLoader<T>::GetUmapWordToIdx(){
  return this->word_to_idx;
};


template <typename T>
typename ContextLoader<T>::umap_str_int ContextLoader<T>::GetCounterFunctionNames(){
  return this->function_name_counter;
};

template <typename T>
typename ContextLoader<T>::umap_str_int ContextLoader<T>::GetCounterPaths(){
  return this->path_counter;
};

template <typename T>
typename ContextLoader<T>::umap_str_int ContextLoader<T>::GetCounterWords(){
  return this->word_counter;
};



}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch