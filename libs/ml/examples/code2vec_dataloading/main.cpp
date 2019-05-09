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

#include "math/tensor.hpp"
#include "ml/dataloaders/code2vec_context_loaders/context_loader.hpp"
#include <fstream>
#include <iostream>

using DataType  = uint64_t;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = fetch::math::Tensor<DataType>::SizeType;

std::string readFile(std::string const &path)
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

  fetch::ml::dataloaders::C2VLoader<std::tuple<ArrayType, ArrayType, ArrayType>, SizeType> cloader(
      20);

  cloader.AddData(readFile(av[1]));
  std::cout << "Number of different function names: " << cloader.GetCounterFunctionNames().size()
            << std::endl;
  std::cout << "Number of different paths: " << cloader.GetCounterPaths().size() << std::endl;
  std::cout << "Number of different words: " << cloader.GetCounterWords().size() << std::endl;

  std::cout << cloader.GetUmapIdxToFunctionName()[0] << std::endl;
  std::cout << cloader.GetUmapIdxToFunctionName()[1] << std::endl;
  std::cout << cloader.GetUmapIdxToFunctionName()[2] << std::endl;

  auto input = cloader.GetNext();

  std::cout << std::get<2>(input.first).ToString() << std::endl;

  return 0;
}
