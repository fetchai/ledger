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

#include "ml/dataloaders/text_loader.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class CBoWLoader : public TextLoader<T>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

private:
  TextParams<T> p_;

public:
  explicit CBoWLoader(TextParams<T> p, SizeType seed = 123456789);
  explicit CBoWLoader(std::string &data, TextParams<T> p, SizeType seed = 123456789);

private:
  virtual std::vector<SizeType> GetData(SizeType idx) override;
};

template <typename T>
CBoWLoader<T>::CBoWLoader(TextParams<T> p, SizeType seed)
  : TextLoader<T>(p, seed)
  , p_(p)
{

  // sanity checks on parameters
  assert(p_.window_size > 0);
}

template <typename T>
CBoWLoader<T>::CBoWLoader(std::string &data, TextParams<T> p, SizeType seed)
  : TextLoader<T>(data, p, seed)
  , p_(p)
{

  // sanity checks on parameters
  assert(p_.window_size > 0);
}

/**
 * get a single training pair from a word index
 * @param idx
 * @return
 */
template <typename T>
std::vector<typename CBoWLoader<T>::SizeType> CBoWLoader<T>::GetData(SizeType idx)
{
  std::vector<typename CBoWLoader<T>::SizeType> ret{};

  // the data assignment - left window
  for (SizeType i = idx - p_.window_size; i < idx; ++i)
  {
    ret.emplace_back(i);
  }

  // the data assignment - right window
  for (SizeType i = idx + 1; i < idx + p_.window_size + 1; ++i)
  {
    ret.emplace_back(i);
  }

  // the label assignment
  ret.emplace_back(idx);

  return ret;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
