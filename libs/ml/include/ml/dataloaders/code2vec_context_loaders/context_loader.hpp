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

  ContextLoader() = default;

  void        AddData(std::string const &text);
  static void addValueToUnorderedMap(std::unordered_map<std::string, int> &umap, std::string word);
  static std::vector<std::string> splitStringByChar(std::stringstream input, const char *sep);

  std::unordered_map<std::string, int> function_name_counter;
  std::unordered_map<std::string, int> path_counter;
  std::unordered_map<std::string, int> word_counter;

private:
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
void ContextLoader<T>::addValueToUnorderedMap(std::unordered_map<std::string, int> &umap,
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

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
