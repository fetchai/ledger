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

#include "math/tensor/tensor.hpp"

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
 * @tparam InputType the type of "feature" vector, the context triple (hashed)
 * @tparam LabelType the type of the "target", i.e. a (hashed) function name
 */
template <typename TensorType>
class C2VLoader : public DataLoader<TensorType>
{
public:
  using DataType                = typename TensorType::Type;
  using SizeType                = fetch::math::SizeType;
  using ContextTuple            = std::tuple<SizeType, SizeType, SizeType>;
  using ContextVector           = std::vector<TensorType>;
  using ContextLabelPair        = std::pair<SizeType, ContextTuple>;
  using ContextTensorsLabelPair = std::pair<TensorType, ContextVector>;

  using WordIdxType   = std::vector<std::vector<SizeType>>;
  using VocabType     = std::unordered_map<std::string, SizeType>;
  using SentencesType = std::vector<std::vector<std::string>>;
  using umap_str_int  = std::unordered_map<std::string, SizeType>;
  using umap_int_str  = std::unordered_map<SizeType, std::string>;

  explicit C2VLoader(SizeType max_contexts_)
    : DataLoader<TensorType>()
    , iterator_position_get_next_context_(0)
    , iterator_position_get_next_(0)
    , current_function_index_(0)
    , max_contexts_(max_contexts_)
  {}

  ContextLabelPair        GetNextContext();
  ContextTensorsLabelPair GetNext() override;

  SizeType Size() const override;
  bool     IsDone() const override;
  void     Reset() override;
  bool     AddData(std::vector<TensorType> const &data, TensorType const &label) override;
  void     SetTestRatio(DataType new_test_ratio) override;
  void     SetValidationRatio(DataType new_validation_ratio) override;
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

  LoaderType LoaderCode() override
  {
    return LoaderType::C2V;
  }

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

  static void addValueToCounter(umap_str_int &umap, const std::string &word);

  static std::vector<std::string> splitStringByChar(std::stringstream input, char const *sep);

  SizeType addToIdxUMaps(std::string const &input, umap_str_int &name_to_idx,
                         umap_int_str &idx_to_name);
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
