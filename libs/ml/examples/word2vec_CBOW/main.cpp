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

#include "math/tensor.hpp"
#include "unigram_table.hpp"
#include "w2v_cbow_dataloader.hpp"

#include "averaged_embeddings.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/transpose.hpp"

#include "ml/graph.hpp"

using namespace fetch::ml;
using namespace fetch::ml::ops;

#define MAX_STRING 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40

typedef float real;  // Precision of float numbers

CBOWLoader<real> global_loader(5);
UnigramTable     unigram_table(0);

char train_file[MAX_STRING], output_file[MAX_STRING];

int          window = 5, num_threads = 1, min_reduce = 1;
unsigned int min_count = 5;

unsigned long long vocab_max_size = 1000, layer1_size = 200;
unsigned long long train_words = 0, word_count_actual = 0, iter = 5, file_size = 0, classes = 0;

real alpha = 0.025f, starting_alpha, sample = 1e-3f;

std::vector<fetch::math::Tensor<real>> syn1neg;  // Weights
real *                                 expTable;
clock_t                                start;

int       hs = 0, negative = 25;
const int table_size = 1e8;
int *     table;

fetch::ml::Graph<fetch::math::Tensor<real>>                    graph;
fetch::ml::ops::AveragedEmbeddings<fetch::math::Tensor<float>> word_vectors_embeddings_module(1, 1);
fetch::ml::ops::Embeddings<fetch::math::Tensor<float>>         word_weights_embeddings_module(1, 1);
fetch::ml::ops::MatrixMultiply<fetch::math::Tensor<float>>     dot_module;
// Add a sigmoid module here -- Not done for now as it would require bringing all the math library

// Keep out for easy saving
fetch::math::Tensor<real> word_embeding_matrix({1, 1});

std::string readFile(std::string const &path)
{
  std::cout << "Reading " << path << std::endl;
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

void InitUnigramTable()
{
  std::vector<uint64_t> frequencies(global_loader.VocabSize());
  for (auto const &kvp : global_loader.GetVocab())
  {
    frequencies[kvp.second.first] = kvp.second.second;
  }
  unigram_table.Reset(table_size, frequencies);
}

void InitNet()
{
  fetch::math::Tensor<real> weight_embeding_matrix({global_loader.VocabSize(), layer1_size});
  word_embeding_matrix = fetch::math::Tensor<real>({global_loader.VocabSize(), layer1_size});
  for (auto &e : word_embeding_matrix)
    e = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) / layer1_size;
  word_vectors_embeddings_module.SetData(word_embeding_matrix);
  word_weights_embeddings_module.SetData(weight_embeding_matrix);

  graph.AddNode<PlaceHolder<fetch::math::Tensor<real>>>("Context", {});
  graph.AddNode<AveragedEmbeddings<fetch::math::Tensor<real>>>(
      "Words", {"Context"}, word_embeding_matrix);  // global_loader.VocabSize(), layer1_size);
  graph.AddNode<PlaceHolder<fetch::math::Tensor<real>>>("Target", {});
  graph.AddNode<Embeddings<fetch::math::Tensor<real>>>(
      "Weights", {"Target"}, weight_embeding_matrix);  // global_loader.VocabSize(), layer1_size);
  graph.AddNode<Transpose<fetch::math::Tensor<real>>>("WeightsTranspose", {"Weights"});
  graph.AddNode<MatrixMultiply<fetch::math::Tensor<real>>>("DotProduct",
                                                           {"Words", "WeightsTranspose"});
}

/**
 * ======== TrainModelThread ========
 * This function performs the training of the model.
 */
void *TrainModelThread(void *id)
{
  // Make a copy of the global loader for thread
  CBOWLoader<float> thread_loader(global_loader);
  // thread_loader.SetOffset(thread_loader.Size() / (long long)num_threads * (long long)id);

  /*
   * word - Stores the index of a word in the vocab table.
   * word_count - Stores the total number of training words processed.
   */
  long long d;
  uint64_t  word;
  real      f;

  fetch::math::Tensor<real> f_tensor({1, uint64_t(negative)});      // Prediction
  fetch::math::Tensor<real> g_tensor({1, uint64_t(negative)});      // Error
  fetch::math::Tensor<real> label_tensor({1, uint64_t(negative)});  // Target Word input

  auto sample = thread_loader.GetNext();

  uint64_t iterations = global_loader.Size();  // / num_threads;
  for (unsigned int i(0); i < iter * iterations; ++i)
  {
    if (id == 0 && i % 10000 == 0)
    {
      alpha = starting_alpha * (((float)iter * iterations - i) / (iter * iterations));
      if (alpha < starting_alpha * 0.0001f)
        alpha = starting_alpha * 0.0001f;
      std::cout << i << " / " << iter * iterations << " (" << (int)(100.0 * i / (iter * iterations))
                << ") -- " << alpha << std::endl;
    }

    if (thread_loader.IsDone())
    {
      std::cout << id << " -- Reset" << std::endl;
      thread_loader.Reset();
    }

    thread_loader.GetNext(sample);
    word = (uint64_t)(sample.second.At(0, 0));

    graph.SetInput("Context", sample.first);
    for (d = 0; d < negative; d++)
    {
      // Fill the target word input -- [0] is positive sample, the rest is negative
      if (d == 0)
        label_tensor.Set(0, d, word);
      else
        label_tensor.Set(0, d, unigram_table.SampleNegative(word));
    }
    graph.SetInput("Target", label_tensor);
    auto graphF = graph.Evaluate("DotProduct");

    // This block computes sigmoid activation + MSE and store error signal in g_tensor
    for (d = 0; d < negative; d++)
    {
      f           = graphF.At(0, d);
      float label = (d == 0) ? 1 : 0;
      if (f > MAX_EXP)
      {
        g_tensor.Set(0, d, label - 1);
      }
      else if (f < -MAX_EXP)
      {
        g_tensor.Set(0, d, label - 0);
      }
      else
      {
        g_tensor.Set(0, d, label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]);
      }
    }
    graph.BackPropagate("DotProduct", g_tensor);
    graph.Step(alpha);
  }
  pthread_exit(NULL);
}

/**
 * ======== TrainModel ========
 * Main entry point to the training process.
 */
void TrainModel()
{
  long               a;
  unsigned long long b;
  FILE *             fo;

  pthread_t *pt = (pthread_t *)malloc(1 * sizeof(pthread_t));

  printf("Starting training using file %s\n", train_file);

  starting_alpha = alpha;

  // Stop here if no output_file was specified.
  if (output_file[0] == 0)
    return;

  // Allocate the weight matrices and initialize them.
  InitNet();

  // If we're using negative sampling, initialize the unigram table, which
  // is used to pick words to use as "negative samples" (with more frequent
  // words being picked more often).
  if (negative > 0)
    InitUnigramTable();

  // Record the start time of training.
  start = clock();

  // Run training, which occurs in the 'TrainModelThread' function.
  for (a = 0; a < num_threads; a++)
    pthread_create(&pt[a], NULL, TrainModelThread, (void *)a);
  for (a = 0; a < num_threads; a++)
    pthread_join(pt[a], NULL);

  fo = fopen(output_file, "wb");
  if (classes == 0)
  {
    // Save the word vectors
    fprintf(fo, "%lu %lld\n", global_loader.VocabSize(), layer1_size);
    auto vocab = global_loader.GetVocab();
    for (auto kvp : vocab)  // for (a = 0; a < vocab_size; a++)
    {
      fprintf(fo, "%s ", kvp.first.c_str());  //	fprintf(fo, "%s ", vocab[a].word);
      for (b = 0; b < layer1_size; b++)
      {
        real v = word_embeding_matrix.At(kvp.second.first, b);
        fwrite(&v, sizeof(real), 1, fo);
      }
      fprintf(fo, "\n");
    }
  }
  fclose(fo);
}

int ArgPos(char *str, int argc, char **argv)
{
  int a;
  for (a = 1; a < argc; a++)
  {
    if (!strcmp(str, argv[a]))
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
  int i;
  if (argc == 1)
    return 0;

  output_file[0] = 0;
  if ((i = ArgPos((char *)"-size", argc, argv)) > 0)
    layer1_size = (uint64_t)(atoi(argv[i + 1]));
  if ((i = ArgPos((char *)"-train", argc, argv)) > 0)
    strcpy(train_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-output", argc, argv)) > 0)
    strcpy(output_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-window", argc, argv)) > 0)
    window = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-sample", argc, argv)) > 0)
    sample = (float)(atof(argv[i + 1]));
  if ((i = ArgPos((char *)"-hs", argc, argv)) > 0)
    hs = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-negative", argc, argv)) > 0)
    negative = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-threads", argc, argv)) > 0)
    num_threads = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-iter", argc, argv)) > 0)
    iter = (uint64_t)(atoi(argv[i + 1]));
  if ((i = ArgPos((char *)"-min-count", argc, argv)) > 0)
    min_count = (unsigned int)(atoi(argv[i + 1]));
  if ((i = ArgPos((char *)"-classes", argc, argv)) > 0)
    classes = (uint64_t)(atoi(argv[i + 1]));

  alpha = 0.05f;  // Initial learning rate

  global_loader.AddData(readFile(train_file));
  global_loader.RemoveInfrequent(min_count);
  std::cout << "Dataloader Vocab Size : " << global_loader.VocabSize() << std::endl;

  expTable = (real *)malloc((EXP_TABLE_SIZE + 1) * sizeof(real));
  for (i = 0; i < EXP_TABLE_SIZE; i++)
  {
    expTable[i] = exp((i / (real)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP);  // Precompute the exp() table
    expTable[i] = expTable[i] / (expTable[i] + 1);  // Precompute f(x) = x / (x + 1)
  }
  TrainModel();
  std::cout << "All done" << std::endl;
  return 0;
}
