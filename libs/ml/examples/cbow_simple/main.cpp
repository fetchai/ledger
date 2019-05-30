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

CBOWLoader<float> global_loader(5, 25);
WordLoader<float> new_loader;

std::string train_file, output_file;

int window    = 5;  // 5
int min_count = 5;

double time_forward  = 0;
double time_exp      = 0;
double time_backward = 0;
double time_step     = 0;

uint64_t                          layer1_size = 200;  // 200
uint64_t                          iter        = 1;
float                             alpha       = static_cast<float>(0.025);
float                             starting_alpha;
high_resolution_clock::time_point last_time;
int                               negative = 25;
#define MAX_EXP 6

std::string readFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

void PrintStats(uint32_t const &i, uint32_t const &iterations)
{
  high_resolution_clock::time_point cur_time = high_resolution_clock::now();

  alpha = starting_alpha * (((float)iter * iterations - i) / (iter * iterations));
  if (alpha < starting_alpha * 0.0001)
  {
    alpha = static_cast<float>(starting_alpha * 0.0001);
  }

  duration<double> time_span = duration_cast<duration<double>>(cur_time - last_time);

  std::cout << i << " / " << iter * iterations << " (" << (int)(100.0 * i / (iter * iterations))
            << ") -- " << alpha << " -- " << 10000. / time_span.count() << " words / sec"
            << std::endl;

  last_time = cur_time;
}

void TrainModelNew()
{
  global_loader.Reset();
  fetch::math::ApproxExpImplementation<0> fexp;

  global_loader.GetNext();
  uint32_t iterations = static_cast<uint32_t>(global_loader.Size());

  // Weights: SetData in N5fetch2ml3ops7WeightsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
  // PlaceHolder: SetData in
  // N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE Weights:
  // Initialise Weights: SetData in
  // N5fetch2ml3ops7WeightsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE PlaceHolder:
  // SetData in N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE

  using ContainerType = typename fetch::math::Tensor<FloatType>::ContainerType;

  fetch::math::Tensor<FloatType> words({layer1_size, 1});

  fetch::math::Tensor<FloatType>     embeddings({layer1_size, global_loader.VocabSize()});
  fetch::math::Tensor<FloatType>     gradient_embeddings({layer1_size, global_loader.VocabSize()});
  std::vector<fetch::math::SizeType> updated_rows_embeddings;

  fetch::math::Tensor<FloatType>     weights({layer1_size, global_loader.VocabSize()});
  fetch::math::Tensor<FloatType>     gradient_weights({layer1_size, global_loader.VocabSize()});
  std::vector<fetch::math::SizeType> updated_rows_weights;

  fetch::math::Tensor<FloatType> target_weights({layer1_size, 25});
  fetch::math::Tensor<FloatType> error_signal({uint64_t(negative), 1});

  {  // Embeddings: Initialise
    fetch::random::LinearCongruentialGenerator rng;
    rng.Seed(42);
    for (auto &e : embeddings)
    {
      e = FloatType(rng.AsDouble() / layer1_size);
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
      for(auto &e :error_signal )
      {
        std::cout << e << ", ";
      }
      std::cout << std::endl;

    }

    if (global_loader.IsDone())
    {
      global_loader.Reset();
    }

    // PlaceHolder: SetData in
    // N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE PlaceHolder:
    // SetData in N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    auto  sample  = global_loader.GetNext();
    auto &context = sample.first;
    auto &target  = sample.second;

    ///////////////////////
    // FORWARD
    ///////////////////////

    // Graph: Evaluate in N5fetch2ml5GraphINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops14MatrixMultiplyIS7_EEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops18AveragedEmbeddingsIS7_EEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops11PlaceHolderIS7_EEEE
    // PlaceHolder: ComputeOutputShape in
    // N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE PlaceHolder:
    // Forward in N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    // Average Embeddings: Forward in
    // N5fetch2ml3ops18AveragedEmbeddingsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE

    // Forward: context -> words
    uint64_t valid_samples(0);
    auto     output_slice = words.View(0);
    bool     clear        = true;
    for (FloatType const &i : context)
    {
      if (i >= 0)
      {
        if (clear)
        {
          Assign(output_slice, embeddings.View(fetch::math::SizeType(i)));
          clear = false;
        }
        else
        {
          auto slice = embeddings.View(fetch::math::SizeType(i));
          PolyfillInlineAdd(output_slice, slice);
        }
        valid_samples++;
      }
    }

    FloatType div = static_cast<FloatType>(valid_samples);
    for (auto &val : output_slice)
    {
      val /= div;
    }

    // words CHECKED AND CORRECT.

    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops10EmbeddingsIS7_EEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops11PlaceHolderIS7_EEEE
    // PlaceHolder: ComputeOutputShape in
    // N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE PlaceHolder:
    // Forward in N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    // Embeddings: Forward in
    // N5fetch2ml3ops10EmbeddingsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE

    // Forward: target -> weights

    uint64_t j = 0;

    for (FloatType const &i : target)
    {
      auto slice1 = target_weights.View(j);
      auto slice2 = weights.View(fetch::math::SizeType(i));

      Assign(slice1, slice2);
      j++;
    }

    // MatrixMultiply: Forward in
    // N5fetch2ml3ops14MatrixMultiplyINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    fetch::math::TransposeDot(target_weights, words, error_signal);

    ///////////////////////
    // ERROR
    ///////////////////////
    for (int d = 0; d < negative; d++)
    {
      float f     = error_signal(d, 0);
      float label = (d == 0) ? 1 : 0;

      if (f > MAX_EXP)
      {
        error_signal.Set(d, 0, label - 1);
      }
      else if (f < -MAX_EXP)
      {
        error_signal.Set(d, 0, label);
      }
      else
      {
        float sm = static_cast<float>(std::exp(f) / (1. + std::exp(f))) ; //static_cast<float>(fexp(f) / (1. + fexp(f)));
        error_signal.Set(d, 0, label - sm);
      }
    }

    ///////////////////////
    // BACKWARD
    ///////////////////////

    // Graph: BackPropagate in N5fetch2ml5GraphINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    // Node: BackPropagate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops14MatrixMultiplyIS7_EEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops18AveragedEmbeddingsIS7_EEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops10EmbeddingsIS7_EEEE
    // MatrixMultiply: Backward in
    // N5fetch2ml3ops14MatrixMultiplyINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    fetch::math::Tensor<FloatType> error_words(words.shape());
    fetch::math::Tensor<FloatType> error_target_weights(target_weights.shape()); // TODO: move
    fetch::math::Dot(target_weights, error_signal, error_words);
    fetch::math::DotTranspose(words, error_signal, error_target_weights);

    // Node: BackPropagate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops18AveragedEmbeddingsIS7_EEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops11PlaceHolderIS7_EEEE
    // Average Embeddings: Backward in
    // N5fetch2ml3ops18AveragedEmbeddingsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE

    auto error_signal_slice = error_words.View(0);
    for (FloatType const &i : context)
    {
      if (i >= 0)
      {
        auto ii = static_cast<fetch::math::SizeType>(i);
        updated_rows_embeddings.push_back(ii);
        auto slice1 = gradient_embeddings.View(ii);

        PolyfillInlineAdd(slice1, error_signal_slice);
      }
    }

    // Node: BackPropagate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops11PlaceHolderIS7_EEEE
    // PlaceHolder: Backward in
    // N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE Node:
    // BackPropagate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops10EmbeddingsIS7_EEEE
    // Node: Evaluate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops11PlaceHolderIS7_EEEE
    // Embeddings: Backward in
    // N5fetch2ml3ops10EmbeddingsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE

    j = 0;
    for (FloatType const &i : target)
    {
      auto ii = fetch::math::SizeType(double(i));
      updated_rows_weights.push_back(ii);

      auto slice1 = gradient_weights.View(fetch::math::SizeType(double(i)));
      auto slice2 = error_target_weights.View(j);

      PolyfillInlineAdd(slice1, slice2);
      
      j++;
    }
    // Node: BackPropagate in
    // N5fetch2ml4NodeINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEENS0_3ops11PlaceHolderIS7_EEEE
    // PlaceHolder: Backward in
    // N5fetch2ml3ops11PlaceHolderINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE

    ///////////////////////
    // STEP
    ///////////////////////

    // Graph: Step in N5fetch2ml5GraphINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    // Embeddings: Step in
    // N5fetch2ml3ops10EmbeddingsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE
    float learningRate       = alpha;
    using VectorRegisterType = typename fetch::math::TensorView<FloatType>::VectorRegisterType;
    fetch::memory::TrivialRange range(0, std::size_t(gradient_weights.height()));
    VectorRegisterType          rate(learningRate);
    VectorRegisterType          zero(static_cast<FloatType>(0));

    for (auto const &r : updated_rows_weights)
    {
      auto input = gradient_weights.View(r);
      auto ret   = weights.View(r); // embeddings.View(r);

      ret.data().in_parallel().Apply(
          range, [rate](VectorRegisterType const &a, VectorRegisterType const &b, VectorRegisterType &c) { c = b + a * rate; },
          input.data(), ret.data());
      input.data().in_parallel().Apply([zero](VectorRegisterType &a) { a = zero; });

    }

    updated_rows_weights.clear();

    // Average Embeddings: Step in
    // N5fetch2ml3ops18AveragedEmbeddingsINS_4math6TensorIfNS_6memory11SharedArrayIfLm4EEEEEEE

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

  alpha = static_cast<float>(0.05);  // Initial learning rate

  std::cout << "New loader" << std::endl;
  new_loader.Load(train_file);
  //  new_loader.RemoveInfrequent(static_cast<uint64_t>(min_count));
  //  new_loader.InitUnigramTable(5);
  //  return 0;
  std::cout << "Old loader" << std::endl;
  global_loader.AddData(readFile(train_file));
  global_loader.RemoveInfrequent(static_cast<uint32_t>(min_count));
  global_loader.InitUnigramTable();
  std::cout << "Dataloader Vocab Size : " << global_loader.VocabSize() << std::endl;

  starting_alpha = alpha;
  TrainModelNew();

  std::cout << "All done" << std::endl;
  return 0;
}
