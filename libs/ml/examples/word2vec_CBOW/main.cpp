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

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <chrono>

#include "math/tensor.hpp"
#include "math/approx_exp.hpp"
#include "unigram_table.hpp"
#include "w2v_cbow_dataloader.hpp"
#include "word_loader.hpp"

#include "averaged_embeddings.hpp"
#include "embeddings.hpp"
#include "graph.hpp"
#include "matrix_multiply.hpp"
#include "placeholder.hpp"
using namespace std::chrono;
using namespace fetch::ml;
using namespace fetch::ml::ops;

using FloatType = float;

CBOWLoader<float> global_loader(5, 25);
WordLoader<float> new_loader;

std::string train_file, output_file;

int window    = 5; // 5
int min_count = 5;

double time_forward = 0;
double time_exp = 0;
double time_backward = 0;
double time_step = 0;


uint64_t layer1_size = 200; //200
uint64_t iter        = 1;
float    alpha       = static_cast<float>(0.025);
float    starting_alpha;
high_resolution_clock::time_point  last_time;

int negative = 25;

fetch::ml::Graph<fetch::math::Tensor<FloatType>> graph;
// Add a sigmoid module here -- Not done for now as it would require bringing
// all the math library

// Keep out for easy saving
fetch::math::Tensor<FloatType> word_embeding_matrix({1, 1});

std::string readFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

void PrintStats(uint32_t const& i, uint32_t const& iterations)
{
  high_resolution_clock::time_point cur_time = high_resolution_clock::now();

  alpha = starting_alpha * (((float)iter * iterations - i) / (iter * iterations));
  if (alpha < starting_alpha * 0.0001)
  {
    alpha = static_cast<float>(starting_alpha * 0.0001);
  }

  duration<double> time_span = duration_cast<duration<double>>(cur_time - last_time);

  std::cout << i << " / " << iter * iterations << " (" << (int)(100.0 * i / (iter * iterations))
            << ") -- " << alpha << " -- " << 10000. / time_span.count() << " words / sec" << std::endl;

  last_time = cur_time;

}


void InitNet()
{
  word_embeding_matrix = fetch::math::Tensor<FloatType>({layer1_size, global_loader.VocabSize()});
  for (auto &e : word_embeding_matrix)
    e = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) / layer1_size;

  graph.AddNode<PlaceHolder<fetch::math::Tensor<FloatType>>>("Context", {});
  graph.AddNode<AveragedEmbeddings<fetch::math::Tensor<FloatType>>>("Words", {"Context"},
                                                                   word_embeding_matrix);
  graph.AddNode<PlaceHolder<fetch::math::Tensor<FloatType>>>("Target", {});
  graph.AddNode<Embeddings<fetch::math::Tensor<FloatType>>>("Weights", {"Target"},
                                                           layer1_size, global_loader.VocabSize());
  graph.AddNode<MatrixMultiply<fetch::math::Tensor<FloatType>>>("DotProduct",
                                                               {"Words", "Weights"});
}
#define MAX_EXP 6
void TrainModel()
{
  fetch::math::Tensor<FloatType> error_signal({uint64_t(negative), 1}); 
  fetch::math::ApproxExpImplementation<0> fexp;
  last_time = high_resolution_clock::now();

  global_loader.GetNext();  // TODO: For compatibility with Pierres code

  uint32_t iterations = static_cast<uint32_t>(global_loader.Size());
  for (uint32_t i(0); i < iter * iterations; ++i)
  {
    if (i % 10000 == 0)
    {
      PrintStats(i, iterations);
      std::cout << "Times: " << time_forward << " " << time_exp << " " << time_backward << " " << time_step << std::endl;
    }

    if (global_loader.IsDone())
    {
      global_loader.Reset();
    }

    auto sample = global_loader.GetNext(); 

    graph.SetInput("Context", sample.first);
    graph.SetInput("Target", sample.second);

    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    auto graphF = graph.Evaluate("DotProduct");

    high_resolution_clock::time_point t1b = high_resolution_clock::now();
    for (int d = 0; d < negative; d++)
    {
      float f     = graphF(d, 0);
      float label = (d == 0) ? 1 : 0;

      if (f > MAX_EXP)
      {
        error_signal.Set(d, 0, label - 1);
      }
      else if (f < -MAX_EXP)
      {
        error_signal.Set(d, 0, label );        
      }
      else
      {
        float sm = static_cast<float>(fexp(f) / (1.+fexp(f)));
        error_signal.Set(d, 0, label - sm); 
      }
    }

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    graph.BackPropagate("DotProduct", error_signal);

    high_resolution_clock::time_point t3 = high_resolution_clock::now();
    graph.Step(alpha);
    high_resolution_clock::time_point t4 = high_resolution_clock::now();

    duration<double> time_span1 = duration_cast<duration<double>>(t1b - t1);
    duration<double> time_span1b = duration_cast<duration<double>>(t2 - t1b);
    duration<double> time_span2 = duration_cast<duration<double>>(t3 - t2);    
    duration<double> time_span3 = duration_cast<duration<double>>(t4 - t3); 

    time_forward  += time_span1.count();
    time_exp  += time_span1b.count();    
    time_backward += time_span2.count();
    time_step += time_span3.count();    
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
  InitNet();
  TrainModel();

  std::cout << "All done" << std::endl;
  return 0;
}
