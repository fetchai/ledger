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
#include "math/tensor.hpp"
#include "math/clustering/knn.hpp"
#include "polyfill.hpp"
#include "unigram_table.hpp"
#include "w2v_cbow_dataloader.hpp"



using namespace std::chrono;
using namespace fetch::ml;
using SizeType = fetch::math::SizeType;
using FloatType = float;

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

class W2VModel
{
private:
  using ArrayType = typename fetch::math::Tensor<FloatType>;
  using ContainerType = typename ArrayType::ContainerType;

  SizeType embeddings_size_;
  SizeType negative_;


  FloatType alpha_;
  FloatType starting_alpha_;

  ArrayType words_;

  ArrayType embeddings_;
  ArrayType gradient_embeddings_;
  std::vector<fetch::math::SizeType> updated_rows_embeddings_;


  ArrayType weights_;
  ArrayType gradient_weights_;
  std::vector<fetch::math::SizeType> updated_rows_weights_;

  ArrayType target_weights_;
  ArrayType error_signal_;

  ArrayType error_words_;
  ArrayType error_target_weights_;


  CBOWLoader<FloatType> & data_loader_;

public:

  W2VModel(SizeType embeddings_size,
          SizeType negative,
          FloatType starting_alpha,
          CBOWLoader<FloatType> & data_loader) :
          embeddings_size_(embeddings_size),
          negative_(negative),
          alpha_(starting_alpha),
          starting_alpha_(starting_alpha),
          data_loader_(data_loader)
  {

    words_ = ArrayType({embeddings_size_, 1});

    embeddings_ = ArrayType({embeddings_size_, data_loader.VocabSize()});
    gradient_embeddings_ = ArrayType({embeddings_size_, data_loader.VocabSize()});

    weights_ = ArrayType({embeddings_size_, data_loader.VocabSize()});
    gradient_weights_ = ArrayType({embeddings_size_, data_loader.VocabSize()});

    target_weights_ = ArrayType({embeddings_size_, 25});
    error_signal_ = ArrayType({uint64_t(negative_), 1});

    error_words_ = ArrayType(words_.shape());
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

  void PrintStats(SizeType const &i, SizeType const& iter, SizeType const &iterations, SizeType const& print_frequency,
                  fetch::math::Tensor<FloatType> const &error_signal, high_resolution_clock::time_point last_time)
  {
    high_resolution_clock::time_point cur_time = high_resolution_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(cur_time - last_time);
    double           total     = static_cast<double>(iter * iterations);
    std::cout << i << " / " << iter * iterations << " (" << static_cast<SizeType>(100.0 * i / total)
              << ") -- " << alpha_ << " -- " << static_cast<double>(print_frequency) / time_span.count() << " words / sec"
              << std::endl;

    std::cout << "error_signal: " << error_signal.ToString() << std::endl;

    last_time = cur_time;
  }

  void Train(SizeType iter, SizeType print_frequency)
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

        PrintStats(i, iter, iterations, print_frequency, error_signal_, last_time);
      }

      if (data_loader_.IsDone())
      {
        data_loader_.Reset();
      }

      // Getting context and target
      auto  sample  = data_loader_.GetNext();
      auto &context = sample.first;
      auto &target  = sample.second;

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
      fetch::math::DotTranspose(words_, error_signal_, error_target_weights_);

      // Average Embeddings: Backward
      auto error_signal_view = error_words_.View(0);
      for (FloatType const &i : context)
      {
        if (i >= 0)
        {
          auto ii = static_cast<fetch::math::SizeType>(i);
          updated_rows_embeddings_.push_back(ii);
          auto view1 = gradient_embeddings_.View(ii);

          PolyfillInlineAdd(view1, error_signal_view);
        }
      }

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
      float learning_rate       = alpha_;
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
            [rate](VectorRegisterType const &a, VectorRegisterType const &b, VectorRegisterType &c) {
              c = b + a * rate;
            },
            input.data(), ret.data());
        input.data().in_parallel().Apply([zero](VectorRegisterType &a) { a = zero; });
      }

      updated_rows_weights_.clear();

      // Average Embeddings: Step
      for (auto const &r : updated_rows_embeddings_)
      {
        auto gradientAccumulationSlice = gradient_embeddings_.View(r);
        auto outputSlice               = embeddings_.View(r);
        auto it1                       = gradientAccumulationSlice.begin();
        auto it2                       = outputSlice.begin();
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

template <typename ArrayType>
void EvalAnalogy(CBOWLoader<FloatType> & data_loader, ArrayType & embeddings)
{
  SizeType k = 5;

  std::string word1 = "italy";
  std::string word2 = "rome";
  std::string word3 = "france";
  std::string word4 = "paris";

  SizeType word1_idx = data_loader.IndexFromWord(word1);
  SizeType word2_idx = data_loader.IndexFromWord(word2);
  SizeType word3_idx = data_loader.IndexFromWord(word3);
  SizeType word4_idx = data_loader.IndexFromWord(word4);

  auto italy_vector  = embeddings.Slice(word1_idx).Copy();
  auto rome_vector   = embeddings.Slice(word2_idx).Copy();
  auto france_vector = embeddings.Slice(word3_idx).Copy();
  auto paris_vector  = embeddings.Slice(word4_idx).Copy();

  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output;

  /// Closest word to Italy ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(embeddings, italy_vector, k);
  std::cout << "Closest word to Italy: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << data_loader.WordFromIndex(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Closest word to France ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(embeddings, france_vector, k);
  std::cout << "Closest word to France: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << data_loader.WordFromIndex(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Closest word to Rome ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(embeddings, rome_vector, k);
  std::cout << "Closest word to Rome: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << data_loader.WordFromIndex(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Closest word to Paris ///
  // cosine distance between every word in vocab and Italy
  output = fetch::math::clustering::KNNCosine(embeddings, paris_vector, k);
  std::cout << "Closest word to Paris: " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << data_loader.WordFromIndex(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Vector Math analogy: Paris - France + Italy should give us rome ///
  // vector math - hopefully target_vector is close to the location of the embedding value for word4
  auto analogy_target_vector_1 = france_vector - paris_vector + italy_vector;
  output =
      fetch::math::clustering::KNNCosine(embeddings, analogy_target_vector_1, k);

  std::cout << "France - Paris + Italy = : " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << data_loader.WordFromIndex(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;

  /// Vector Math analogy: Paris - France + Italy should give us rome ///
  // vector math - hopefully target_vector is close to the location of the embedding value for word4
  auto analogy_target_vector_2 = paris_vector - france_vector + italy_vector;
  output =
      fetch::math::clustering::KNNCosine(embeddings, analogy_target_vector_2, k);

  std::cout << "Paris - France + Italy = : " << std::endl;
  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "rank: " << j << ", "
              << "distance, " << output.at(j).second << ": " << data_loader.WordFromIndex(output.at(j).first)
              << std::endl;
  }
  std::cout << std::endl;
}


void SaveEmbeddings(CBOWLoader<FloatType> const& data_loader, std::string output_filename, fetch::math::Tensor<FloatType> &embeddings, bool binary)
{
  FILE *fo = std::fopen(output_filename.c_str(), "wb");

  SizeType embeddings_size = embeddings.shape()[0];
  SizeType vocab_size = embeddings.shape()[1];

  fprintf(fo, "%llu %llu\n", vocab_size, embeddings_size);
  for (SizeType a = 0; a < data_loader.VocabSize(); a++)
  {
    fprintf(fo, "%s ", (data_loader.WordFromIndex(a)).c_str());
    if (binary)
    {
      for (SizeType b = 0; b < embeddings_size; b++)
      {
        std::fwrite(&embeddings(b, a), sizeof(FloatType), 1, fo);
      }
    }
    else
    {
      for (SizeType b = 0; b < embeddings_size; b++)
      {
        fprintf(fo, "%lf ", embeddings(b, a));
      }
    }
    fprintf(fo, "\n");
  }
}

int main(int argc, char **argv)
{
  /// INPUT ARGUMENTS ///
  if (argc == 1)
  {
    std::cout << "must specify training text document as first argument: " << std::endl;
    return 0;
  }

  std::string train_file = argv[1];
  std::string mode = "cbow";
  if (argc == 3)
  {
    mode = argv[2];
  }
  assert((mode == "cbow") || (mode == "skipgram"));

  /// DATA LOADING ///
  SizeType window_size = 8;
  SizeType negative = 25;
  SizeType min_count = 5;

  CBOWLoader<FloatType> data_loader(window_size, negative);

  std::cout << "Loading ..." << std::endl;
  data_loader.AddData(ReadFile(train_file));
  data_loader.RemoveInfrequent(min_count);
  data_loader.InitUnigramTable();
  std::cout << "Dataloader Vocab Size : " << data_loader.VocabSize() << std::endl;

  /// TRAIN MODEL ///

  SizeType embeddings_size = 200;  // embeddings size
  SizeType iter           = 1;    // training epochs
  FloatType alpha = static_cast<FloatType>(0.025);
  SizeType print_frequency = 10000;
  std::string output_file = "./vector.bin";

  std::cout << "Training " << mode << std::endl;
  W2VModel w2v(embeddings_size, negative, alpha, data_loader);
  w2v.Train(iter, print_frequency);
  auto embeddings = w2v.Embeddings();

  /// SHOW ANALOGY EXAMPLE ///
  EvalAnalogy(data_loader, embeddings);

  /// SAVE EMBEDDINGS ///
  bool binary_save_mode = true;
  SaveEmbeddings(data_loader, output_file, embeddings, binary_save_mode);
  std::cout << "All done" << std::endl;

  return 0;
}
