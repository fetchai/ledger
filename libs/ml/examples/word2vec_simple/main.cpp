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

#include "math/approx_exp.hpp"
#include "math/clustering/knn.hpp"
#include "math/exceptions/exceptions.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/w2v_dataloader.hpp"
#include "polyfill.hpp"
#include "w2v_model.hpp"
#include "word_analogy.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using FloatType  = float;
using SizeType   = fetch::math::SizeType;
using TensorType = fetch::math::Tensor<FloatType>;

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  if (t.fail())
  {
    throw fetch::ml::exceptions::InvalidFile("Cannot open file " + path);
  }

  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

void SaveEmbeddings(W2VLoader<FloatType> const &data_loader, std::string const &output_filename,
                    TensorType &embeddings)
{
  std::ofstream outfile(output_filename, std::ios::binary);

  SizeType embeddings_size = embeddings.shape()[0];
  SizeType vocab_size      = embeddings.shape()[1];

  outfile << std::to_string(embeddings_size) << " " << std::to_string(vocab_size) << "\n";
  for (SizeType a = 0; a < data_loader.vocab_size(); a++)
  {
    outfile << data_loader.WordFromIndex(a) << " ";
    for (SizeType b = 0; b < embeddings_size; ++b)
    {
      outfile << embeddings(b, a) << " ";
    }
    outfile << "\n";
  }
}

TensorType LoadEmbeddings(std::string const &filename)
{

  std::ifstream input(filename, std::ios::binary);
  if (input.fail())
  {
    throw exceptions::InvalidFile("embeddings file does not exist");
  }

  std::string line;
  input >> line;
  SizeType embeddings_size = std::stoull(line);

  input >> line;
  SizeType vocab_size = std::stoull(line);

  std::cout << "embeddings_size: " << embeddings_size << std::endl;
  std::cout << "vocab_size: " << vocab_size << std::endl;

  TensorType embeddings({embeddings_size, vocab_size});

  std::string cur_word;
  std::string cur_string;
  for (SizeType a = 0; a < vocab_size; a++)
  {
    input >> cur_word;

    for (SizeType b = 0; b < embeddings_size; ++b)
    {
      input >> cur_string;
      if (cur_string != "\n")
      {
        embeddings(b, a) = std::stof(cur_string);
      }
      else
      {
        break;
      }
    }
  }
  return embeddings;
}

int ArgPos(char const *str, int argc, char **argv)
{
  int a;
  for (a = 1; a < argc; a++)
  {
    if (strcmp(str, argv[a]) == 0)
    {
      if (a == argc - 1)
      {
        printf("Argument missing for %s\n", str);
        exit(1);
      }
      return a;
    }
  }
  return -1;
}

int main(int argc, char **argv)
{
  int                      i;
  std::string              train_file;
  bool                     load = false;  // whether to load or train new embeddings
  bool                     mode = true;   // true for cbow, false for sgns
  std::string              output_file;
  SizeType                 top_k           = 10;
  std::vector<std::string> test_words      = {"france", "paris", "italy"};
  SizeType                 window_size     = 8;
  SizeType                 negative        = 25;
  SizeType                 min_count       = 5;
  SizeType                 embeddings_size = 200;  // embeddings size
  SizeType                 iter            = 15;   // training epochs
  auto                     alpha           = static_cast<FloatType>(0.025);
  SizeType                 print_frequency = 10000;

  /// INPUT ARGUMENTS ///
  if ((i = ArgPos("-train", argc, argv)) > 0)
  {
    train_file = argv[i + 1];
  }
  if ((i = ArgPos("-mode", argc, argv)) > 0)
  {
    assert((std::string(argv[i + 1]) == "cbow") || (std::string(argv[i + 1]) == "sgns"));
    mode = std::string(argv[i + 1]) == "cbow";
  }
  if ((i = ArgPos("-output", argc, argv)) > 0)
  {
    output_file = argv[i + 1];
  }
  if ((i = ArgPos("-k", argc, argv)) > 0)
  {
    top_k = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos("-word1", argc, argv)) > 0)
  {
    test_words[0] = argv[i + 1];
  }
  if ((i = ArgPos("-word2", argc, argv)) > 0)
  {
    test_words[1] = argv[i + 1];
  }
  if ((i = ArgPos("-word3", argc, argv)) > 0)
  {
    test_words[2] = argv[i + 1];
  }
  if ((i = ArgPos("-window", argc, argv)) > 0)
  {
    window_size = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos("-negative", argc, argv)) > 0)
  {
    negative = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos("-min", argc, argv)) > 0)
  {
    min_count = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos("-embedding", argc, argv)) > 0)
  {
    embeddings_size = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos("-epochs", argc, argv)) > 0)
  {
    iter = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos("-lr", argc, argv)) > 0)
  {
    alpha = std::stof(argv[i + 1]);
  }
  if ((i = ArgPos("-print", argc, argv)) > 0)
  {
    print_frequency = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos("-load", argc, argv)) > 0)
  {
    load = (std::stoull(argv[i + 1]) != 0u);
  }

  // if no train file specified - we just run analogy example
  TensorType embeddings;

  W2VLoader<FloatType> data_loader(window_size, negative);

  if (!load)
  {
    /// DATA LOADING ///
    std::cout << "building vocab " << std::endl;
    data_loader.BuildVocab(ReadFile(train_file));
    data_loader.RemoveInfrequent(min_count);
    data_loader.InitUnigramTable();
    std::cout << "Vocab Size : " << data_loader.vocab_size() << std::endl;

    /// SAVE VOCAB///
    std::cout << "saving vocab " << std::endl;
    data_loader.SaveVocab("vocab_" + output_file);

    /// TRAIN EMBEDDINGS ///
    if (static_cast<int>(mode) == 1)
    {
      std::cout << "Training CBOW" << std::endl;
    }
    else
    {
      std::cout << "Training SGNS" << std::endl;
    }

    std::cout << "training embeddings " << std::endl;
    W2VModel<TensorType> w2v(embeddings_size, negative, alpha, data_loader);
    w2v.Train(iter, print_frequency, mode);
    embeddings = w2v.Embeddings();

    /// SAVE EMBEDDINGS ///
    std::cout << "saving embeddings: " << std::endl;
    SaveEmbeddings(data_loader, "embed_" + output_file, embeddings);
  }
  else
  {
    /// LOAD VOCAB ///
    std::cout << "loading vocab " << std::endl;
    data_loader.LoadVocab("vocab_" + output_file);

    /// LOAD EMBEDDINGS FROM A FILE ///
    std::cout << "Loading embeddings" << std::endl;
    embeddings = LoadEmbeddings("embed_" + output_file);
  }

  /// SHOW ANALOGY EXAMPLE ///
  EvalAnalogy(data_loader, embeddings, top_k, test_words);

  return 0;
}
