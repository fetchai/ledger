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
#include "polyfill.hpp"
#include "unigram_table.hpp"
#include "w2v_cbow_dataloader.hpp"
#include "word_loader.hpp"

using namespace std::chrono;
using namespace fetch::ml;
using FloatType = float;

CBOWLoader<FloatType> data_loader(5, 25);
WordLoader<FloatType> new_loader;

std::string train_file, output_file;

int32_t window    = 5;  // 5
int32_t min_count = 5;

double time_forward  = 0;
double time_exp      = 0;
double time_backward = 0;
double time_step     = 0;

uint64_t                          dimensionality = 200;  // 200
uint64_t                          iter           = 1;
FloatType                         alpha          = static_cast<FloatType>(0.025);
FloatType                         starting_alpha;
int32_t                           negative = 25;
high_resolution_clock::time_point last_time;

std::string readFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

void PrintStats(uint32_t const &i, uint32_t const &iterations)
{
  high_resolution_clock::time_point cur_time = high_resolution_clock::now();

  alpha =
      starting_alpha * ((static_cast<FloatType>(iter * iterations) - static_cast<FloatType>(i)) /
                        static_cast<FloatType>(iter * iterations));
  if (alpha < starting_alpha * 0.0001)
  {
    alpha = static_cast<float>(starting_alpha * 0.0001);
  }

  duration<double> time_span = duration_cast<duration<double>>(cur_time - last_time);
  double           total     = static_cast<double>(iter * iterations);
  std::cout << i << " / " << iter * iterations << " (" << static_cast<int32_t>(100.0 * i / total)
            << ") -- " << alpha << " -- " << 10000. / time_span.count() << " words / sec"
            << std::endl;

  last_time = cur_time;
}

void TrainModelNew()
{
  data_loader.Reset();
  fetch::math::ApproxExpImplementation<0> fexp;

  data_loader.GetNext();
  uint32_t iterations = static_cast<uint32_t>(data_loader.Size());

  // Initialising
  using ContainerType = typename fetch::math::Tensor<FloatType>::ContainerType;

  fetch::math::Tensor<FloatType> words({dimensionality, 1});

  fetch::math::Tensor<FloatType>     embeddings({dimensionality, data_loader.VocabSize()});
  fetch::math::Tensor<FloatType>     gradient_embeddings({dimensionality, data_loader.VocabSize()});
  std::vector<fetch::math::SizeType> updated_rows_embeddings;

  fetch::math::Tensor<FloatType>     weights({dimensionality, data_loader.VocabSize()});
  fetch::math::Tensor<FloatType>     gradient_weights({dimensionality, data_loader.VocabSize()});
  std::vector<fetch::math::SizeType> updated_rows_weights;

  fetch::math::Tensor<FloatType> target_weights({dimensionality, 25});
  fetch::math::Tensor<FloatType> error_signal({uint64_t(negative), 1});

  fetch::math::Tensor<FloatType> error_words(words.shape());
  fetch::math::Tensor<FloatType> error_target_weights(target_weights.shape());

  {  // Embeddings: Initialise
    fetch::random::LinearCongruentialGenerator rng;
    rng.Seed(42);
    for (auto &e : embeddings)
    {
      e = FloatType(rng.AsDouble() / static_cast<double>(dimensionality));
    }
  }
  {  // Weights: Initialise
    fetch::random::LinearCongruentialGenerator rng;
    rng.Seed(42);
    fetch::math::TensorSliceIterator<FloatType, ContainerType> it(weights);
    while (it.is_valid())
    {
      *it = FloatType(rng.AsDouble());
      ++it;
    }
  }

  for (uint32_t i = 0; i < iter * iterations; ++i)
  {

    if (i % 10000 == 0)
    {
      PrintStats(i, iterations);

      std::cout << "Times: " << time_forward << " " << time_exp << " " << time_backward << " "
                << time_step << std::endl;
      std::cout << "      -- ";
      for (auto &e : error_signal)
      {
        std::cout << e << ", ";
      }
      std::cout << std::endl;
    }

    if (data_loader.IsDone())
    {
      data_loader.Reset();
    }

    // Getting context and target
    auto  sample  = data_loader.GetNext();
    auto &context = sample.first;
    auto &target  = sample.second;

    ///////////////////////
    // FORWARD
    ///////////////////////

    // Average Embeddings: context -> words
    uint64_t valid_samples(0);
    auto     output_view = words.View(0);
    bool     clear       = true;
    for (FloatType const &i : context)
    {
      if (i >= 0)
      {
        if (clear)
        {
          Assign(output_view, embeddings.View(fetch::math::SizeType(i)));
          clear = false;
        }
        else
        {
          auto view = embeddings.View(fetch::math::SizeType(i));
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
      auto view1 = target_weights.View(j);
      auto view2 = weights.View(fetch::math::SizeType(i));

      Assign(view1, view2);
      j++;
    }

    // MatrixMultiply: Forward
    fetch::math::TransposeDot(target_weights, words, error_signal);

    ///////////////////////
    // ERROR
    ///////////////////////
    for (int32_t d = 0; d < negative; d++)
    {
      FloatType f     = error_signal(d, 0);
      FloatType label = (d == 0) ? 1 : 0;
      FloatType sm    = static_cast<FloatType>(static_cast<FloatType>(fexp(f) / (1. + fexp(f))));
      error_signal.Set(d, 0, label - sm);
    }

    ///////////////////////
    // BACKWARD
    ///////////////////////

    // MatrixMultiply: Backward
    fetch::math::Dot(target_weights, error_signal, error_words);
    fetch::math::DotTranspose(words, error_signal, error_target_weights);

    // Average Embeddings: Backward
    auto error_signal_view = error_words.View(0);
    for (FloatType const &i : context)
    {
      if (i >= 0)
      {
        auto ii = static_cast<fetch::math::SizeType>(i);
        updated_rows_embeddings.push_back(ii);
        auto view1 = gradient_embeddings.View(ii);

        PolyfillInlineAdd(view1, error_signal_view);
      }
    }

    // Embeddings: Backward
    j = 0;
    for (FloatType const &i : target)
    {
      auto ii = fetch::math::SizeType(double(i));
      updated_rows_weights.push_back(ii);

      auto view1 = gradient_weights.View(fetch::math::SizeType(double(i)));
      auto view2 = error_target_weights.View(j);

      PolyfillInlineAdd(view1, view2);

      j++;
    }

    ///////////////////////
    // STEP
    ///////////////////////

    // Embeddings: Step in
    float learningRate       = alpha;
    using VectorRegisterType = typename fetch::math::TensorView<FloatType>::VectorRegisterType;
    fetch::memory::TrivialRange range(0, std::size_t(gradient_weights.height()));
    VectorRegisterType          rate(learningRate);
    VectorRegisterType          zero(static_cast<FloatType>(0));

    for (auto const &r : updated_rows_weights)
    {
      auto input = gradient_weights.View(r);
      auto ret   = weights.View(r);  // embeddings.View(r);

      ret.data().in_parallel().Apply(
          range,
          [rate](VectorRegisterType const &a, VectorRegisterType const &b, VectorRegisterType &c) {
            c = b + a * rate;
          },
          input.data(), ret.data());
      input.data().in_parallel().Apply([zero](VectorRegisterType &a) { a = zero; });
    }

    updated_rows_weights.clear();

    // Average Embeddings: Step
    for (auto const &r : updated_rows_embeddings)
    {
      auto gradientAccumulationSlice = gradient_embeddings.View(r);
      auto outputSlice               = embeddings.View(r);
      auto it1                       = gradientAccumulationSlice.begin();
      auto it2                       = outputSlice.begin();
      while (it1.is_valid())
      {
        *it2 += (*it1 * learningRate);
        *it1 = 0;
        ++it1;
        ++it2;
      }
    }

    updated_rows_embeddings.clear();
  }

  std::cout << "Done" << std::endl;
}

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    return 0;
  }

  output_file = "./vector.bin";
  train_file  = argv[1];

  alpha = static_cast<FloatType>(0.05);  // Initial learning rate

  //  std::cout << "New loader" << std::endl;
  //  new_loader.Load(train_file);

  std::cout << "Old loader" << std::endl;
  data_loader.AddData(readFile(train_file));
  data_loader.RemoveInfrequent(static_cast<uint32_t>(min_count));
  data_loader.InitUnigramTable();
  std::cout << "Dataloader Vocab Size : " << data_loader.VocabSize() << std::endl;

  starting_alpha = alpha;
  TrainModelNew();

  std::cout << "All done" << std::endl;
  return 0;
}
