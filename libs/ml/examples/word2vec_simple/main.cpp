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

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "math/approx_exp.hpp"
#include "math/clustering/knn.hpp"
#include "math/tensor.hpp"
#include "polyfill.hpp"
#include "unigram_table.hpp"
#include "w2v_dataloader.hpp"
#include "word_analogy.hpp"

using namespace std::chrono;
using namespace fetch::ml;
using SizeType  = fetch::math::SizeType;
using FloatType = float;

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

class W2VModel
{
private:
  using ArrayType     = typename fetch::math::Tensor<FloatType>;
  using ContainerType = typename ArrayType::ContainerType;

  SizeType embeddings_size_;
  SizeType negative_;

  FloatType alpha_;
  FloatType starting_alpha_;

  ArrayType words_;

  ArrayType                          embeddings_;
  ArrayType                          gradient_embeddings_;
  std::vector<fetch::math::SizeType> updated_rows_embeddings_;

  ArrayType                          weights_;
  ArrayType                          gradient_weights_;
  std::vector<fetch::math::SizeType> updated_rows_weights_;

  ArrayType target_weights_;
  ArrayType error_signal_;

  ArrayType error_words_;
  ArrayType error_target_weights_;

  W2VLoader<FloatType> &data_loader_;

public:
  W2VModel(SizeType embeddings_size, SizeType negative, FloatType starting_alpha,
           W2VLoader<FloatType> &data_loader)
    : embeddings_size_(embeddings_size)
    , negative_(negative)
    , alpha_(starting_alpha)
    , starting_alpha_(starting_alpha)
    , data_loader_(data_loader)
  {

    words_ = ArrayType({embeddings_size_, 1});

    embeddings_          = ArrayType({embeddings_size_, data_loader.VocabSize()});
    gradient_embeddings_ = ArrayType({embeddings_size_, data_loader.VocabSize()});

    weights_          = ArrayType({embeddings_size_, data_loader.VocabSize()});
    gradient_weights_ = ArrayType({embeddings_size_, data_loader.VocabSize()});

    target_weights_ = ArrayType({embeddings_size_, 25});
    error_signal_   = ArrayType({uint64_t(negative_), 1});

    error_words_          = ArrayType(words_.shape());
    error_target_weights_ = ArrayType(target_weights_.shape());

    {  // Embeddings: Initialise
      fetch::random::LinearCongruentialGenerator rng;
      rng.Seed(42);
      for (auto &e : embeddings_)
      {
        e = FloatType((rng.AsDouble() - 0.5) / static_cast<double>(embeddings_size));
      }
    }
    {  // Weights: Initialise
      fetch::random::LinearCongruentialGenerator rng;
      rng.Seed(42);
      fetch::math::TensorSliceIterator<FloatType, ContainerType> it(weights_);
      while (it.is_valid())
      {
        *it = FloatType(rng.AsDouble());
        ++it;
      }
    }
  }

  void PrintStats(SizeType const &i, SizeType const &iter, SizeType const &iterations,
                  SizeType const &print_frequency, high_resolution_clock::time_point &last_time)
  {
    high_resolution_clock::time_point cur_time = high_resolution_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(cur_time - last_time);
    double           total     = static_cast<double>(iter * iterations);
    std::cout << i << " / " << iter * iterations << " (" << static_cast<SizeType>(100.0 * i / total)
              << ") -- " << alpha_ << " -- "
              << static_cast<double>(print_frequency) / time_span.count() << " words / sec"
              << std::endl;

    last_time = cur_time;
  }

  void Train(SizeType iter, SizeType print_frequency, bool cbow = 1)
  {

    high_resolution_clock::time_point last_time;

    data_loader_.Reset();
    fetch::math::ApproxExpImplementation<0> fexp;

    data_loader_.GetNext();
    SizeType iterations = data_loader_.Size();

    // training loop
    for (SizeType i = 0; i < iter * iterations; ++i)
    {

      if (i % print_frequency == 0)
      {
        alpha_ = starting_alpha_ *
                 ((static_cast<FloatType>(iter * iterations) - static_cast<FloatType>(i)) /
                  static_cast<FloatType>(iter * iterations));
        if (alpha_ < starting_alpha_ * 0.0001)
        {
          alpha_ = static_cast<float>(starting_alpha_ * 0.0001);
        }

        PrintStats(i, iter, iterations, print_frequency, last_time);
      }

      if (data_loader_.IsDone())
      {
        data_loader_.Reset();
      }

      // Getting context and target
      auto  sample  = data_loader_.GetNext();
      auto &context = sample.first;
      auto &target  = sample.second;

      if (cbow)
      {
        ///////////////////////
        // FORWARD
        ///////////////////////

        // Average Embeddings: context -> words
        uint64_t valid_samples(0);
        auto     output_view = words_.View(0);
        bool     clear       = true;
        for (FloatType const &i : context)
        {
          if (i >= 0)
          {
            if (clear)
            {
              Assign(output_view, embeddings_.View(fetch::math::SizeType(i)));
              clear = false;
            }
            else
            {
              auto view = embeddings_.View(fetch::math::SizeType(i));
              PolyfillInlineAdd(output_view, view);
            }
            valid_samples++;
          }
        }

        FloatType div = static_cast<FloatType>(valid_samples);
        for (auto &val : output_view)
        {
          val /= div;
        }
      }

      // Embeddings: target -> weights
      uint64_t j = 0;

      for (FloatType const &i : target)
      {
        auto view1 = target_weights_.View(j);
        auto view2 = weights_.View(fetch::math::SizeType(i));

        Assign(view1, view2);
        j++;
      }

      // MatrixMultiply: Forward
      fetch::math::TransposeDot(target_weights_, words_, error_signal_);

      ///////////////////////
      // ERROR
      ///////////////////////
      for (SizeType d = 0; d < negative_; d++)
      {
        FloatType f     = error_signal_(d, 0);
        FloatType label = (d == 0) ? 1 : 0;
        FloatType sm    = static_cast<FloatType>(static_cast<FloatType>(fexp(f) / (1. + fexp(f))));
        error_signal_.Set(d, 0, label - sm);
      }

      ///////////////////////
      // BACKWARD
      ///////////////////////

      // MatrixMultiply: Backward
      fetch::math::Dot(target_weights_, error_signal_, error_words_);

      // TODO: for cbow the shape is diffeernt because of weight averaging
      fetch::math::DotTranspose(words_, error_signal_, error_target_weights_);

      // Embeddings: Backward
      j = 0;
      for (FloatType const &i : target)
      {
        auto ii = fetch::math::SizeType(double(i));
        updated_rows_weights_.push_back(ii);

        auto view1 = gradient_weights_.View(fetch::math::SizeType(double(i)));
        auto view2 = error_target_weights_.View(j);

        PolyfillInlineAdd(view1, view2);

        j++;
      }

      ///////////////////////
      // STEP
      ///////////////////////

      // Embeddings: Step in
      float learning_rate      = alpha_;
      using VectorRegisterType = typename fetch::math::TensorView<FloatType>::VectorRegisterType;
      fetch::memory::TrivialRange range(0, std::size_t(gradient_weights_.height()));
      VectorRegisterType          rate(learning_rate);
      VectorRegisterType          zero(static_cast<FloatType>(0));

      for (auto const &r : updated_rows_weights_)
      {
        auto input = gradient_weights_.View(r);
        auto ret   = weights_.View(r);  // embeddings.View(r);

        ret.data().in_parallel().Apply(
            range,
            [rate](VectorRegisterType const &a, VectorRegisterType const &b,
                   VectorRegisterType &c) { c = b + a * rate; },
            input.data(), ret.data());
        input.data().in_parallel().Apply([zero](VectorRegisterType &a) { a = zero; });
      }

      updated_rows_weights_.clear();

      // Embeddings: Step
      for (auto const &r : updated_rows_embeddings_)
      {
        auto it1 = (gradient_embeddings_.View(r)).begin();
        auto it2 = (embeddings_.View(r)).begin();
        while (it1.is_valid())
        {
          *it2 += (*it1 * learning_rate);
          *it1 = 0;
          ++it1;
          ++it2;
        }
      }

      updated_rows_embeddings_.clear();
    }

    std::cout << "Done Training" << std::endl;
  }

  ArrayType Embeddings()
  {
    return embeddings_;
  }
};

void SaveEmbeddings(W2VLoader<FloatType> const &data_loader, std::string output_filename,
                    fetch::math::Tensor<FloatType> &embeddings)
{
  std::ofstream outfile(output_filename, std::ios::binary);

  SizeType embeddings_size = embeddings.shape()[0];
  SizeType vocab_size      = embeddings.shape()[1];

  outfile << std::to_string(embeddings_size) << " " << std::to_string(vocab_size) << "\n";
  for (SizeType a = 0; a < data_loader.VocabSize(); a++)
  {
    outfile << data_loader.WordFromIndex(a) << " ";
    for (SizeType b = 0; b < embeddings_size; ++b)
    {
      outfile << embeddings(b, a) << " ";
    }
    outfile << "\n";
  }
}

///**
// * A method for saving in the style the original google code wants to load
// * @param data_loader
// * @param output_filename
// * @param embeddings
// */
// void SaveEmbeddingsOld(W2VLoader<FloatType> const& data_loader, std::string output_filename,
// fetch::math::Tensor<FloatType> &embeddings)
//{
//  SizeType embeddings_size = embeddings.shape()[0];
//  SizeType vocab_size = embeddings.shape()[1];
//
//  FILE * fo = std::fopen(output_filename.c_str(), "wb");
//  // Save the word vectors
//  fprintf(fo, "%lld %lld\n", vocab_size, embeddings_size);
//  for (SizeType a = 0; a < vocab_size; a++)
//  {
//    fprintf(fo, "%s ", data_loader.WordFromIndex(a).c_str());
//    for (SizeType b = 0; b < embeddings_size; b++)
//    {
//      std::fwrite(&embeddings(b, a), sizeof(FloatType), 1, fo);
//    }
//
//    fprintf(fo, "\n");
//  }
//}

fetch::math::Tensor<FloatType> LoadEmbeddings(std::string filename)
{

  std::ifstream input(filename, std::ios::binary);

  std::string line;
  input >> line;
  std::cout << "embed line: " << line << std::endl;
  SizeType embeddings_size = std::stoull(line);

  input >> line;
  std::cout << "vocab line: " << line << std::endl;
  SizeType vocab_size = std::stoull(line);

  std::cout << "embeddings_size: " << embeddings_size << std::endl;
  std::cout << "vocab_size: " << vocab_size << std::endl;

  fetch::math::Tensor<FloatType> embeddings({embeddings_size, vocab_size});

  std::string cur_word;
  std::string cur_string;
  for (SizeType a = 0; a < vocab_size; a++)
  {
    input >> cur_word;

    // TODO - also load words into vocab of dataloader?
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

int ArgPos(char *str, int argc, char **argv)
{
  int a;
  for (a = 1; a < argc; a++)
    if (!strcmp(str, argv[a]))
    {
      if (a == argc - 1)
      {
        printf("Argument missing for %s\n", str);
        exit(1);
      }
      return a;
    }
  return -1;
}

int main(int argc, char **argv)
{
  int                      i;
  std::string              train_file      = "";
  bool                     load            = false;  // whether to load or train new embeddings
  bool                     train_mode      = true;   // true for cbow, false for sgns
  std::string              output_file     = "./vector.bin";
  SizeType                 top_k           = 10;
  std::vector<std::string> test_words      = {"france", "paris", "italy"};
  SizeType                 window_size     = 8;
  SizeType                 negative        = 25;
  SizeType                 min_count       = 5;
  SizeType                 embeddings_size = 200;  // embeddings size
  SizeType                 iter            = 15;   // training epochs
  FloatType                alpha           = static_cast<FloatType>(0.025);
  SizeType                 print_frequency = 10000;

  /// INPUT ARGUMENTS ///
  if ((i = ArgPos((char *)"-train", argc, argv)) > 0)
  {
    train_file = argv[i + 1];
  }
  if ((i = ArgPos((char *)"-mode", argc, argv)) > 0)
  {
    assert((argv[i + 1] == "cbow") || (argv[i + 1] == "sgns"));
    if (std::string(argv[i + 1]) == "cbow")
    {
      train_mode = true;
    }
    else
    {
      train_mode = false;
    }
  }
  if ((i = ArgPos((char *)"-output", argc, argv)) > 0)
  {
    output_file = argv[i + 1];
  }
  if ((i = ArgPos((char *)"-k", argc, argv)) > 0)
  {
    top_k = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-word1", argc, argv)) > 0)
  {
    test_words[0] = argv[i + 1];
  }
  if ((i = ArgPos((char *)"-word2", argc, argv)) > 0)
  {
    test_words[1] = argv[i + 1];
  }
  if ((i = ArgPos((char *)"-word3", argc, argv)) > 0)
  {
    test_words[2] = argv[i + 1];
  }
  if ((i = ArgPos((char *)"-window", argc, argv)) > 0)
  {
    window_size = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-negative", argc, argv)) > 0)
  {
    negative = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-min", argc, argv)) > 0)
  {
    min_count = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-embedding", argc, argv)) > 0)
  {
    embeddings_size = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-epochs", argc, argv)) > 0)
  {
    iter = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-lr", argc, argv)) > 0)
  {
    alpha = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-print", argc, argv)) > 0)
  {
    print_frequency = std::stoull(argv[i + 1]);
  }
  if ((i = ArgPos((char *)"-load", argc, argv)) > 0)
  {
    load = std::stoull(argv[i + 1]);
  }

  // if no train file specified - we just run analogy example
  fetch::math::Tensor<FloatType> embeddings;

  // TODO : make vocab saveable
  std::cout << "Loading ..." << std::endl;
  W2VLoader<FloatType> data_loader(window_size, negative, train_mode);
  data_loader.AddData(ReadFile(train_file));
  data_loader.RemoveInfrequent(min_count);
  data_loader.InitUnigramTable();
  std::cout << "Dataloader Vocab Size : " << data_loader.VocabSize() << std::endl;

  if (!load)
  {
    /// DATA LOADING ///

    if (train_mode == 1)
    {
      std::cout << "Training CBOW" << std::endl;
    }
    else
    {
      std::cout << "Training SGNS" << std::endl;
    }

    W2VModel w2v(embeddings_size, negative, alpha, data_loader);
    w2v.Train(iter, print_frequency, train_mode);
    embeddings = w2v.Embeddings();

    /// SAVE EMBEDDINGS ///
    SaveEmbeddings(data_loader, output_file, embeddings);
    //  SaveEmbeddingsOld(data_loader, "./vector_old.bin", embeddings);
  }
  else
  {
    /// LOAD EMBEDDINGS FROM A FILE ///
    std::cout << "Loading existing embeddings from: " << output_file << std::endl;
    embeddings = LoadEmbeddings(output_file);
  }

  /// SHOW ANALOGY EXAMPLE ///
  EvalAnalogy(data_loader, embeddings, top_k, test_words);

  return 0;
}