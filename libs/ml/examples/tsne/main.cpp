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

#include <math/tensor.hpp>

#include "ml/clustering/tsne.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

using namespace fetch::math;
using namespace fetch::ml;

using DataType     = double;
using ArrayType    = Tensor<DataType>;

#include <math/tensor.hpp>
#include <sstream>

/**
 * i.e. Fill tensor matrix with DataType values from file at path
 * @param matrix output matrix to be filled
 * @param path input Path to file with values
 */
void ReadFile(Tensor<DataType> &matrix, std::string const &path)
{

  std::ifstream source;                  // build a read-Stream
  source.open(path, std::ios_base::in);  // open data

  if (!source)
  {  // if it does not work
    std::cerr << "Can't open file: " << path << std::endl;
  }

  for (SizeType i = 0; i < matrix.shape().at(0); i++)
  {
    std::string line;
    std::getline(source, line);
    std::istringstream in(line);  // make a stream for the line itself

    for (SizeType j = 0; j < matrix.shape().at(1); j++)
    {
      DataType num;
      in >> num;
      matrix.Set(i, j, num);
    }
  }
}

int main(int ac, char **av)
{
  SizeType SUBSET_SIZE                  = 100;
  SizeType RANDOM_SEED                  = 123456;
  DataType     LEARNING_RATE                = 500;
  SizeType MAX_ITERATIONS               = 100;
  DataType     PERPLEXITY                   = 20;
  SizeType N_OUTPUT_FEATURE_SIZE        = 2;
  DataType     INITIAL_MOMENTUM             = 0.5;
  DataType     FINAL_MOMENTUM               = 0.8;
  SizeType FINAL_MOMENTUM_STEPS         = 20;
  SizeType P_LATER_CORRECTION_ITERATION = 10;

  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::cout << "Loading input data. " << std::endl;
  fetch::ml::MNISTLoader<ArrayType> dataloader(av[1], av[2]);
  std::pair<ArrayType, ArrayType>   input = dataloader.SubsetToArray(SUBSET_SIZE);

  // Initialize TSNE
  std::cout << "Running TSNE init. " << std::endl;
  TSNE<Tensor<DataType>> tsn(input.first, N_OUTPUT_FEATURE_SIZE, PERPLEXITY, RANDOM_SEED);

  std::cout << "Started optimization. " << std::endl;
  tsn.Optimize(LEARNING_RATE, MAX_ITERATIONS, INITIAL_MOMENTUM, FINAL_MOMENTUM,
               FINAL_MOMENTUM_STEPS, P_LATER_CORRECTION_ITERATION);

  std::cout << "Result: " << tsn.GetOutputMatrix().ToString() << std::endl;
  std::cout << "Finished! " << std::endl;
  return 0;
}
