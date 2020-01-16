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
#include "ml/dataloaders/code2vec_context_loaders/context_loader.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

constexpr std::size_t MAX_CONTEXTS = 20;

using DataType   = int64_t;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::Tensor<DataType>::SizeType;

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

int main(int ac, char **av)
{
  if (ac < 2)
  {
    std::cerr << "Usage: " << av[0] << " INPUT_FILES_TXT" << std::endl;
    return 1;
  }

  fetch::ml::dataloaders::C2VLoader<TensorType> cloader(MAX_CONTEXTS);

  cloader.AddDataAsString(ReadFile(av[1]));
  std::cout << "Number of different function names: " << cloader.function_name_counter().size()
            << std::endl;
  std::cout << "Number of different paths: " << cloader.path_counter().size() << std::endl;
  std::cout << "Number of different words: " << cloader.word_counter().size() << std::endl;

  std::cout << "Retrieving function names from cloader" << std::endl;
  std::cout << cloader.umap_idx_to_functionname().at(0) << std::endl;
  std::cout << cloader.umap_idx_to_functionname().at(1) << std::endl;
  std::cout << cloader.umap_idx_to_functionname().at(2) << std::endl;

  auto input = cloader.GetNext();
  std::cout << "Getting next input indices" << std::endl;
  std::cout << input.second.at(2).ToString() << std::endl;

  return 0;
}
