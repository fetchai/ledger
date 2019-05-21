//  Copyright 2013 Google Inc. All Rights Reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

// ---------------------------------------------------------------------
//
// Word2vec C code commented by Chandler May.  Run
//
//   git diff original
//
// to see the complete set of changes to the original code.
//
// Preliminaries:
// * There are a lot of global variables.  They are important.
// * a, b, and c are used for loop counters and lots of other things;
//   it's not uncommon for one of them to be set to one thing and then
//   re-initialized and used for something completely different.
// * Random number generation is through a custom linear congruential
//   generator.  This in particular facilitates decoupled random number
//   generation across training threads.
//
// Notation:
// * Backticks are used to denote variables (`foo`)
// * The skip-gram model learns co-occurrence information between a
//   word and a fixed-length window of context words on either side of
//   it.  The embedding of the central word is called the "input"
//   representation and the embeddings of the left and right context
//   words are "output" representations.  This distinction (and
//   terminology) are quite important for understanding the code so here
//   is an example, assumping a context window of two words on either
//   side:
//
//     The quick brown fox jumped over the lazy dog.
//                ^     ^    ^     ^    ^
//                |     |    |     |    |
//           output output input output output
//
//   When looking at the input word "jumped" in this sentence, the
//   skip-gram with negative sampling (SGNS) model learns that it
//   co-occurs with "brown" and "fox" and "over" and "the" and does not
//   co-occur with a selection of negative-sample words (perhaps
//   "apple", "of", and "slithered").
//
// Partial call graph:
//
//   main
//   |
//   L- TrainModel
//      |
//      |- ReadVocab
//      |  |- ReadWord
//      |  |- AddWordToVocab
//      |  L- SortVocab
//      |
//      |- LearnVocabFromTrainFile
//      |  |- ReadWord
//      |  |- AddWordToVocab
//      |  |- SearchVocab
//      |  |- ReduceVocab
//      |  L- SortVocab
//      |
//      |- SaveVocab
//      |- InitNet
//      |- InitUnigramTable
//      |
//      L- TrainModelThread
//         L- ReadWordIndex
//            |- ReadWord
//            L- SearchVocab
//
// ---------------------------------------------------------------------

#include <iostream>

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// max length of filenames, vocabulary words (including null terminator)
#define MAX_STRING 100
// size of pre-computed e^x / (e^x + 1) table
#define EXP_TABLE_SIZE 1000
// max exponent x for which to pre-compute e^x / (e^x + 1)
#define MAX_EXP 6
// max length (in words) of a sequence of overlapping word contexts or
// "sentences" (if there is a sequence of words longer than this not
// explicitly broken up into multiple "sentences" in the training data,
// it will be broken up into multiple "sentences" of length no longer
// than `MAX_SENTENCE_LENGTH` each during training)
#define MAX_SENTENCE_LENGTH 1000
// max length of Huffman codes used by hierarchical softmax
#define MAX_CODE_LENGTH 40

// Maximum 30 * 0.7 = 21M words in the vocabulary
// (where 0.7 is the magical load factor beyond which hash table
// performance degrades)
const int vocab_hash_size = 30000000;

// Negative sampling distribution represented by 1e8-element discrete
// sample from smoothed empirical unigram distribution.
const int table_size = 1e8;

// Set precision of real numbers
typedef float real;

// Representation of a word in the vocabulary, including (optional,
// for hierarchical softmax only) Huffman coding
struct vocab_word
{
  long long cn;
  int *     point;
  char *    word, *code, codelen;
};

struct vocab_word *vocab;                   // vocabulary
char               train_file[MAX_STRING],  // training data (text) input file
    output_file[MAX_STRING],                // word vector (or word vector cluster)
                                            //   (binary/text) output file
    save_vocab_file[MAX_STRING],            // vocabulary (text) output file
    read_vocab_file[MAX_STRING];            // vocabulary (text) input file
int binary     = 0,                         // 0 for text output, 1 for binary
    cbow       = 1,                         // 0 for skip-gram, 1 for CBOW
    debug_mode = 2,                         // 1 for extra terminal output, 2 for
                                            //   extra extra terminal output
    window = 5,                             // context window radius (use `window`
                                            //   output words on left and `window`
                                            //   output words on right as context
                                            //   of central input word)
    min_count = 5,                          // min count of words in vocabulary when
                                            //   pruning to mitigate sparsity
    num_threads = 12,                       // number of threads to use for training
    min_reduce  = 1,                        // initial min count of words to keep in
                                            //   vocabulary if pruning for space
                                            //   (will be incremented as necessary)
                                            //   (do not change)
    hs       = 0,                           // 1 for hierarchical softmax
    negative = 5;                           // number of negative samples to draw
                                            //   per word
int *vocab_hash,                            // hash table of words to positions in
                                            //   vocabulary
    *table;                                 // discrete sample of words used as
                                            //   negative sampling distribution
long long vocab_max_size = 1000,            // capacity of vocabulary
                                            //   (will be incremented as necessary)
    vocab_size = 0,                         // number of words in vocabulary
                                            //   (do not change)
    layer1_size = 100,                      // size of embeddings
    train_words = 0,                        // number of word tokens in training data
                                            //   (do not change)
    word_count_actual = 0,                  // number of word tokens seen so far
                                            //   during training, over all
                                            //   iterations; updated infrequently
                                            //   (used for terminal output and
                                            //   learning rate)
                                            //   (do not change)
    iter = 5,                               // number of passes to take through
                                            //   training data
    file_size = 0,                          // size (in bytes) of training data file
    classes   = 0;                          // number of k-means clusters to learn
                                            //   of word vectors and write to output
                                            //   file (0 to write word vectors to
                                            //   output file, no clustering)
real alpha = float(0.025),                  // linear-decay learning rate
    starting_alpha,                         // initial learning rate
                                            //   (do not change; initialized at
                                            //   beginning of training)
    sample = float(1e-3);                   // word subsampling threshold
real *syn0,                                 // input word embeddings
    *syn1,                                  // (used by hierarchical softmax)
    *syn1neg,                               // output word embeddings
    *expTable;                              // precomputed table of
                                            //   e^x / (e^x + 1) for x in
                                            //   [-MAX_EXP, MAX_EXP)
clock_t start;                              // start time of training algorithm

// Allocate and populate negative-sampling data structure `table`, an
// array of `table_size` words distributed approximately according to
// the empirical unigram distribution (smoothed by raising all
// probabilities to the power of 0.75 and re-normalizing), from `vocab`,
// an array of `vocab_size` words represented as `vocab_word` structs.
void InitUnigramTable()
{
  int    a, i;
  double train_words_pow = 0;
  double d1, power = 0.75;
  // allocate memory
  table = (int *)malloc(table_size * sizeof(int));
  // compute normalizer, `train_words_pow`
  for (a = 0; a < vocab_size; a++)
    train_words_pow += pow(vocab[a].cn, power);
  // initialize vocab position `i` and cumulative probability mass `d1`
  i  = 0;
  d1 = pow(vocab[i].cn, power) / train_words_pow;
  for (a = 0; a < table_size; a++)
  {
    table[a] = i;
    if (a / (double)table_size > d1)
    {
      i++;
      d1 += pow(vocab[i].cn, power) / train_words_pow;
    }
    if (i >= vocab_size)
    {
      i = static_cast<int>(vocab_size - 1);
    }
  }
}

// Read a single word from file `fin` into length `MAX_STRING` array
// `word`, terminating with a null byte, treating space, tab, and
// newline as word boundaries.  Ignore carriage returns.  If the first
// character read (excluding carriage returns) is a newline, set `word`
// to "</s>" and return.  If a newline is encountered after reading one
// or more non-boundary characters, put that newline back into the
// stream and leave word as the non-boundary characters up to that point
// (so that the next time this function is called the newline will be
// read and `word` will be set to "</s>").  If the number of
// non-boundary characters is equal to or greater than `MAX_STRING`,
// read and ignore the trailing characters.  If we reach end of file,
// set `eof` to 1.
void ReadWord(char *word, FILE *fin, char *eof)
{
  int a = 0, ch;
  while (1)
  {
    ch = getc_unlocked(fin);
    if (ch == EOF)
    {
      *eof = 1;
      break;
    }
    if (ch == '\r')
      continue;  // skip carriage returns
    if ((ch == ' ') || (ch == '\t') || (ch == '\n'))
    {
      if (a > 0)
      {
        if (ch == '\n')
          ungetc(ch, fin);
        break;
      }
      if (ch == '\n')
      {
        strcpy(word, (char *)"</s>");
        return;
      }
      else
        continue;
    }
    word[a] = static_cast<char>(ch);
    a++;
    if (a >= MAX_STRING - 1)
      a--;  // Truncate too-long words
  }
  word[a] = 0;
}

// Return hash (integer between 0, inclusive, and `vocab_hash_size`,
// exclusive) of `word`
int GetWordHash(char *word)
{
  unsigned long long a, hash = 0;
  for (a = 0; a < strlen(word); a++)
    hash = hash * 257 + static_cast<unsigned long long>(word[a]);
  hash = hash % vocab_hash_size;
  return static_cast<int>(hash);
}

// Return position of `word` in vocabulary `vocab` using `vocab_hash`,
// a linear-probing hash table; if the word is not found, return -1.
int SearchVocab(char *word)
{
  // compute initial hash
  unsigned int hash = static_cast<unsigned int>(GetWordHash(word));
  while (1)
  {
    // return -1 if cell is empty
    if (vocab_hash[hash] == -1)
      return -1;
    // return position at current hash if word is a match
    if (!strcmp(word, vocab[vocab_hash[hash]].word))
      return vocab_hash[hash];
    // no match, increment hash
    hash = (hash + 1) % vocab_hash_size;
  }
  return -1;
}

// Read a word from file `fin` and return its position in vocabulary
// `vocab`.  If the next thing in the file is a newline, return 0 (the
// index of "</s>").  If the word is not in the vocabulary, return -1.
// If the end of file is reached, set `eof` to 1 and return -1.  TODO:
// if the file is not newline-terminated, the last word will be
// swallowed (-1 will be returned because we have reached EOF, even if
// a word was read).
int ReadWordIndex(FILE *fin, char *eof)
{
  char word[MAX_STRING], eof_l = 0;
  ReadWord(word, fin, &eof_l);
  if (eof_l)
  {
    *eof = 1;
    return -1;
  }
  return SearchVocab(word);
}

// Add word `word` to first empty slot in vocabulary `vocab`, increment
// vocabulary length `vocab_size`, increase `vocab_max_size` by
// increments of 1000 (increasing `vocab` allocation) as needed, and set
// position of that slot in vocabulary hash table `vocab_hash`.
// Return index of `word` in `vocab`.
int AddWordToVocab(char *word)
{
  unsigned int hash, length = static_cast<unsigned int>(strlen(word) + 1);
  if (length > MAX_STRING)
    length = MAX_STRING;
  // add word to `vocab` and increment `vocab_size`
  vocab[vocab_size].word = (char *)calloc(length, sizeof(char));
  // TODO: this may blow up if strlen(word) + 1 > MAX_STRING
  strcpy(vocab[vocab_size].word, word);
  vocab[vocab_size].cn = 0;
  vocab_size++;
  // Reallocate memory if needed
  if (vocab_size + 2 >= vocab_max_size)
  {
    vocab_max_size += 1000;
    vocab = (struct vocab_word *)realloc(
        vocab, static_cast<unsigned long long>(vocab_max_size) * sizeof(struct vocab_word));
  }
  // add word vocabulary position to `vocab_hash`
  hash = static_cast<unsigned int>(GetWordHash(word));
  while (vocab_hash[hash] != -1)
    hash = (hash + 1) % vocab_hash_size;
  vocab_hash[hash] = static_cast<int>(vocab_size - 1);
  return static_cast<int>(vocab_size - 1);
}

// Given pointers `a` and `b` to `vocab_word` structs, return 1 if the
// stored count `cn` of b is greater than that of a, -1 if less than,
// and 0 if equal.  (Comparator for a reverse sort by word count.)
int VocabCompare(const void *a, const void *b)
{
  long long l = ((struct vocab_word *)b)->cn - ((struct vocab_word *)a)->cn;
  if (l > 0)
    return 1;
  if (l < 0)
    return -1;
  return 0;
}

// Sort vocabulary `vocab` by word count, decreasing, while removing
// words that have count less than `min_count`; re-compute `vocab_hash`
// accordingly; shrink vocab memory allocation to minimal size.
void SortVocab()
{
  int          a, size;
  unsigned int hash;
  // Sort the vocabulary but keep "</s>" at the first position
  qsort(&vocab[1], static_cast<size_t>(vocab_size - 1), sizeof(struct vocab_word), VocabCompare);
  // clear `vocab_hash` cells
  for (a = 0; a < vocab_hash_size; a++)
    vocab_hash[a] = -1;
  // save initial `vocab_size` (we will decrease `vocab_size` as we
  // prune infrequent words)
  size = static_cast<int>(vocab_size);
  // initialize total word count
  train_words = 0;
  for (a = 0; a < size; a++)
  {
    if ((vocab[a].cn < min_count) && (a != 0))
    {
      // word is infrequent and not "</s>", discard it
      vocab_size--;
      free(vocab[a].word);
    }
    else
    {
      // word is frequent or "</s>", add to hash table
      hash = static_cast<unsigned int>(GetWordHash(vocab[a].word));
      while (vocab_hash[hash] != -1)
        hash = (hash + 1) % vocab_hash_size;
      vocab_hash[hash] = a;
      train_words += vocab[a].cn;
    }
  }
  // shrink vocab memory allocation to minimal size
  // TODO: to be safe we should probably update vocab_max_size which
  // seems to be interpreted as the allocation size
  vocab = (struct vocab_word *)realloc(
      vocab, static_cast<unsigned long long>(vocab_size + 1) * sizeof(struct vocab_word));
  // Allocate memory for the binary tree construction
  for (a = 0; a < vocab_size; a++)
  {
    vocab[a].code  = (char *)calloc(MAX_CODE_LENGTH, sizeof(char));
    vocab[a].point = (int *)calloc(MAX_CODE_LENGTH, sizeof(int));
  }
}

// Reduce vocabulary `vocab` size by removing words with count equal to
// `min_reduce` or less, in order to make room in hash table (not for
// mitigating data sparsity).  Increment `min_reduce` by one, so that
// this function can be called in a loop until there is enough space.
void ReduceVocab()
{
  int          a, b = 0;
  unsigned int hash;
  for (a = 0; a < vocab_size; a++)
    if (vocab[a].cn > min_reduce)
    {
      vocab[b].cn   = vocab[a].cn;
      vocab[b].word = vocab[a].word;
      b++;
    }
    else
      free(vocab[a].word);
  vocab_size = b;
  for (a = 0; a < vocab_hash_size; a++)
    vocab_hash[a] = -1;
  // recompute `vocab_hash` as we have removed some items and it may
  // now be broken
  for (a = 0; a < vocab_size; a++)
  {
    hash = static_cast<unsigned int>(GetWordHash(vocab[a].word));
    while (vocab_hash[hash] != -1)
      hash = (hash + 1) % vocab_hash_size;
    vocab_hash[hash] = a;
  }
  fflush(stdout);
  min_reduce++;
}

// Create binary Huffman tree from word counts in `vocab`, storing
// codes in `vocab`; frequent words will have short uniqe binary codes.
// Used by hierarchical softmax.
void CreateBinaryTree()
{
  long long  a, b, i, min1i, min2i, pos1, pos2, point[MAX_CODE_LENGTH];
  char       code[MAX_CODE_LENGTH];
  long long *count =
      (long long *)calloc(static_cast<size_t>(vocab_size * 2 + 1), sizeof(long long));
  long long *binary =
      (long long *)calloc(static_cast<size_t>(vocab_size * 2 + 1), sizeof(long long));
  long long *parent_node =
      (long long *)calloc(static_cast<size_t>(vocab_size * 2 + 1), sizeof(long long));
  for (a = 0; a < vocab_size; a++)
    count[a] = vocab[a].cn;
  for (a = vocab_size; a < vocab_size * 2; a++)
    count[a] = 1e15;
  pos1 = vocab_size - 1;
  pos2 = vocab_size;
  // Following algorithm constructs the Huffman tree by adding one node at a time
  for (a = 0; a < vocab_size - 1; a++)
  {
    // First, find two smallest nodes 'min1, min2'
    if (pos1 >= 0)
    {
      if (count[pos1] < count[pos2])
      {
        min1i = pos1;
        pos1--;
      }
      else
      {
        min1i = pos2;
        pos2++;
      }
    }
    else
    {
      min1i = pos2;
      pos2++;
    }
    if (pos1 >= 0)
    {
      if (count[pos1] < count[pos2])
      {
        min2i = pos1;
        pos1--;
      }
      else
      {
        min2i = pos2;
        pos2++;
      }
    }
    else
    {
      min2i = pos2;
      pos2++;
    }
    count[vocab_size + a] = count[min1i] + count[min2i];
    parent_node[min1i]    = vocab_size + a;
    parent_node[min2i]    = vocab_size + a;
    binary[min2i]         = 1;
  }
  // Now assign binary code to each vocabulary word
  for (a = 0; a < vocab_size; a++)
  {
    b = a;
    i = 0;
    while (1)
    {
      code[i]  = static_cast<char>(binary[b]);
      point[i] = b;
      i++;
      b = parent_node[b];
      if (b == vocab_size * 2 - 2)
        break;
    }
    vocab[a].codelen  = static_cast<char>(i);
    vocab[a].point[0] = static_cast<int>(vocab_size - 2);
    for (b = 0; b < i; b++)
    {
      vocab[a].code[i - b - 1] = code[b];
      vocab[a].point[i - b]    = static_cast<int>(point[b] - vocab_size);
    }
  }
  free(count);
  free(binary);
  free(parent_node);
}

// Compute vocabulary `vocab` and corresponding hash table `vocab_hash`
// from text in `train_file`.  Insert </s> as vocab item 0.  Prune vocab
// incrementally (as needed to keep number of items below effective hash
// table capacity).  After reading file, sort vocabulary by word count,
// decreasing.
void LearnVocabFromTrainFile()
{
  char      word[MAX_STRING], eof = 0;
  FILE *    fin;
  long long a, i, wc = 0;
  for (a = 0; a < vocab_hash_size; a++)
    vocab_hash[a] = -1;
  fin = fopen(train_file, "rb");
  if (fin == NULL)
  {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  vocab_size = 0;
  AddWordToVocab((char *)"</s>");
  while (1)
  {
    // TODO: if file is not newline-terminated, last word may be
    // swallowed
    ReadWord(word, fin, &eof);
    if (eof)
      break;
    train_words++;
    wc++;
    if ((debug_mode > 1) && (wc >= 1000000))
    {
      printf("%lldM%c", train_words / 1000000, 13);
      fflush(stdout);
      wc = 0;
    }
    i = SearchVocab(word);
    if (i == -1)
    {
      a           = AddWordToVocab(word);
      vocab[a].cn = 1;
    }
    else
    {
      vocab[i].cn++;
    }
    if (vocab_size > static_cast<long long>(static_cast<double>(vocab_hash_size) * 0.7))
    {
      ReduceVocab();
    }
  }
  SortVocab();
  if (debug_mode > 0)
  {
    printf("Vocab size: %lld\n", vocab_size);
    printf("Words in train file: %lld\n", train_words);
  }
  file_size = ftell(fin);
  fclose(fin);
}

// Write vocabulary `vocab` to file `save_vocab_file`, one word per
// line, with each line containing a word, a space, the word count, and
// a newline.
void SaveVocab()
{
  long long i;
  FILE *    fo = fopen(save_vocab_file, "wb");
  for (i = 0; i < vocab_size; i++)
    fprintf(fo, "%s %lld\n", vocab[i].word, vocab[i].cn);
  fclose(fo);
}

// Read vocabulary from file `read_vocab_file` that has one word per
// line, where each line contains a word, a space, the word count, and a
// newline.  Store vocabulary in `vocab` and update `vocab_hash`
// accordingly.  After reading, sort vocabulary by word count,
// decreasing.
void ReadVocab()
{
  long long a, i   = 0;
  char      c, eof = 0;
  char      word[MAX_STRING];
  FILE *    fin = fopen(read_vocab_file, "rb");
  if (fin == NULL)
  {
    printf("Vocabulary file not found\n");
    exit(1);
  }
  for (a = 0; a < vocab_hash_size; a++)
    vocab_hash[a] = -1;
  vocab_size = 0;
  while (1)
  {
    // TODO: if file is not newline-terminated, last word may be
    // swallowed
    ReadWord(word, fin, &eof);
    if (eof)
      break;
    a = AddWordToVocab(word);
    fscanf(fin, "%lld%c", &vocab[a].cn, &c);
    i++;
  }
  SortVocab();
  if (debug_mode > 0)
  {
    printf("Vocab size: %lld\n", vocab_size);
    printf("Words in train file: %lld\n", train_words);
  }
  fin = fopen(train_file, "rb");
  if (fin == NULL)
  {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  file_size = ftell(fin);
  fclose(fin);
}

// Allocate memory for and initialize neural network parameters.  Each
// array has size `vocab_size` x `layer1_size`.
//
//   syn0: input word embeddings, initialized uniformly on
//         [-0.5/`layer1_size`, 0.5/`layer1_size`)
//   syn1: only used by hierarchical softmax; initialized to 0
//   syn1neg: output word embeddings; initialized to zero
void InitNet()
{
  long long          a, b;
  unsigned long long next_random = 1;
  a                              = posix_memalign((void **)&syn0, 128,
                     static_cast<unsigned long long>(vocab_size * layer1_size) * sizeof(real));
  if (syn0 == NULL)
  {
    printf("Memory allocation failed\n");
    exit(1);
  }
  if (hs)
  {
    a = posix_memalign((void **)&syn1, 128,
                       static_cast<unsigned long long>(vocab_size * layer1_size) * sizeof(real));
    if (syn1 == NULL)
    {
      printf("Memory allocation failed\n");
      exit(1);
    }
    for (a = 0; a < vocab_size; a++)
      for (b = 0; b < layer1_size; b++)
        syn1[a * layer1_size + b] = 0;
  }
  if (negative > 0)
  {
    a = posix_memalign((void **)&syn1neg, 128,
                       static_cast<unsigned long long>(vocab_size * layer1_size) * sizeof(real));
    if (syn1neg == NULL)
    {
      printf("Memory allocation failed\n");
      exit(1);
    }
    for (a = 0; a < vocab_size; a++)
      for (b = 0; b < layer1_size; b++)
        syn1neg[a * layer1_size + b] = 0;
  }
  for (a = 0; a < vocab_size; a++)
    for (b = 0; b < layer1_size; b++)
    {
      next_random               = next_random * (unsigned long long)25214903917 + 11;
      syn0[a * layer1_size + b] = ((static_cast<float>(static_cast<int>(next_random) & 0xFFFF) /
                                    static_cast<float>(65536)) -
                                   static_cast<float>(0.5)) /
                                  static_cast<float>(layer1_size);
    }
  CreateBinaryTree();
}

// Given allocated and initialized vocabulary `vocab`, corresponding
// hash `vocab_hash`, and neural network parameters `syn0`, `syn1`, and
// `syn1neg`, train word2vec model on 1 / `num_threads` fraction of text
// in `train_file` (storing learned parameters in `syn0`, `syn1`, and
// `syn1neg`).  When cast to long long, `id` should be an integer
// between 0 (inclusive) and `num_threads` (exclusive) representing
// which training thread this is.
//
// Note main loop is broken down into procedures for hierarchical
// softmax versus negative sampling and those for continuous BOW versus
// skip-gram; ensure you are looking in the right code block (for your
// purposes)!  The added comments focus on the SGNS case.
void *TrainModelThread(void *id)
{
  long long a,                             // loop counter among other things
      b,                                   // offset of dynamic window in max window
                                           //   (if 0, use all `window` output
                                           //   words on each side of input word;
                                           //   otherwise use `window - b` words
                                           //   on each side)
      d,                                   // loop counter among other things
      cw,                                  // (used by CBOW)
      word,                                // index of output word in vocabulary
      last_word,                           // index of input word in vocabulary
      sentence_length = 0,                 // number of words read into the current
                                           //   sentence
      sentence_position = 0,               // position of current output word in
                                           //   sentence
      word_count = 0,                      // number of words seen so far in this
                                           //   iteration
      last_word_count = 0,                 // number of words seen so far in this
                                           //   iteration, as of the most recent
                                           //   update of terminal output and learning
                                           //   rate
      l1,                                  // input word row offset in syn0
      l2,                                  // output word row offset in syn1, syn1neg
      c,                                   // loop counter among other things
      target,                              // index of output word or negatively-sampled
                                           //   word in vocabulary
      label,                               // switch between output word (1) and
                                           //   negatively-sampled word (0)
      local_iter = iter;                   // iterations over this thread's chunk of the
                                           //   data set left
  long long sen[MAX_SENTENCE_LENGTH + 1];  // index of word in vocabulary for
                                           //   each word in current sentence
  unsigned long long next_random = (unsigned long long)id;  // thread-specific RNG state
  char               eof         = 0;                       // 1 if end of file has been reached
  real               f, g;  // work space (values of sub-expressions in
                            //   gradient computation)
  clock_t now;              // current time during training
  // allocate memory for gradients
  real *neu1  = (real *)calloc(static_cast<size_t>(layer1_size), sizeof(real)),
       *neu1e = (real *)calloc(static_cast<size_t>(layer1_size), sizeof(real));
  FILE *fi    = fopen(train_file, "rb");

  fseek(fi, file_size / (long long)num_threads * (long long)id, SEEK_SET);

  // iteratively read a sentence and train (update gradients) over it;
  // read over all sentences in this thread's chunk of the training
  // data `iter` times, then break
  while (1)
  {
    // every 10k words, update progress in terminal and update learning
    // rate
    if (word_count - last_word_count > 10000)
    {
      word_count_actual += word_count - last_word_count;
      last_word_count = word_count;
      if ((debug_mode > 1))
      {
        now = clock();
        printf("%cAlpha: %f  Progress: %.2f%%  Words/thread/sec: %.2fk  ", 13, alpha,
               word_count_actual / static_cast<float>(static_cast<float>(iter * train_words) + 1.0) * 100,
               word_count_actual / (static_cast<float>(now - start + 1.0) / static_cast<float>(CLOCKS_PER_SEC) * 1000.0));
        fflush(stdout);
      }
      // linear-decay learning rate (decreases from one toward zero
      // linearly in number of words seen, but thresholded below
      // at one ten-thousandth of initial learning rate)
      // (each word in the data is to be seen `iter` times)
      alpha = starting_alpha * (1 - word_count_actual / (real)(iter * train_words + 1));
      if (alpha < starting_alpha * 0.0001)
        alpha = static_cast<float>(starting_alpha * 0.0001);
    }

    // if we have finished training on the most recently-read sentence
    // (or we are just starting training), read a new sentence;
    // truncate each sentence at `MAX_SENTENCE_LENGTH` words (sentences
    // longer than that will be broken up into smaller sentences)
    if (sentence_length == 0)
    {
      // iteratively read word and add to sentence
      while (1)
      {
        word = ReadWordIndex(fi, &eof);
        if (eof)
          break;
        // skip OOV
        if (word == -1)
          continue;
        word_count++;
        // if EOS, we're done reading this sentence
        if (word == 0)
          break;
        // The subsampling randomly discards frequent words while keeping the ranking same
        if (sample > 0)
        {
          // discard w.p. 1 - [ sqrt(t / p_{word}) + t / p_{word} ]
          // (t is the subsampling threshold `sample`, p_{word} is the
          // ML estimate of the probability of `word` in a unigram LM
          // (normalized frequency)
          // TODO: why is this not merely 1 - sqrt(t / p_{word}) as in
          // the paper?
          real ran = (static_cast<float>(sqrt(vocab[word].cn) / (sample * static_cast<float>(train_words))) + 1) * (sample * static_cast<float>(train_words)) /
                     static_cast<float>(vocab[word].cn);
          next_random = next_random * (unsigned long long)25214903917 + 11;
          if (ran < static_cast<float>(next_random & 0xFFFF) / (real)65536)
            continue;
        }

        sen[sentence_length] = word;
        sentence_length++;
        // truncate long sentences
        if (sentence_length >= MAX_SENTENCE_LENGTH)
          break;
      }

      // set output word position to first word in sentence
      sentence_position = 0;
    }

    // if we are at the end of this iteration (sweep over the data),
    // restart and decrement `local_iter`
    if (eof || (word_count > train_words / num_threads))
    {
      word_count_actual += word_count - last_word_count;
      local_iter--;
      if (local_iter == 0)
        break;
      word_count      = 0;
      last_word_count = 0;
      // signal to read new sentence
      sentence_length = 0;
      fseek(fi, file_size / (long long)num_threads * (long long)id, SEEK_SET);
      continue;
    }

    // get index of output word
    word = sen[sentence_position];
    // skip OOV (TODO, checked OOV already when reading sentence?)
    if (word == -1)
      continue;
    // reset gradients to zero
    for (c = 0; c < layer1_size; c++)
      neu1[c] = 0;
    for (c = 0; c < layer1_size; c++)
      neu1e[c] = 0;
    // pick dynamic window offset (uniformly at random, between 0
    // (inclusive) and max window size `window` (exclusive))
    next_random = next_random * (unsigned long long)25214903917 + 11;
    b           = next_random % static_cast<unsigned int>(window);

    // CBOW
    if (cbow)
    {
      // in -> hidden
      cw = 0;
      for (a = b; a < window * 2 + 1 - b; a++)
        if (a != window)
        {
          c = sentence_position - window + a;
          if (c < 0)
            continue;
          if (c >= sentence_length)
            continue;
          last_word = sen[c];
          if (last_word == -1)
            continue;
          for (c = 0; c < layer1_size; c++)
            neu1[c] += syn0[c + last_word * layer1_size];
          cw++;
        }
      if (cw)
      {
        for (c = 0; c < layer1_size; c++)
          neu1[c] /= static_cast<float>(cw);

        // CBOW HIERARCHICAL SOFTMAX
        if (hs)
          for (d = 0; d < vocab[word].codelen; d++)
          {
            f  = 0;
            l2 = vocab[word].point[d] * layer1_size;
            // Propagate hidden -> output
            for (c = 0; c < layer1_size; c++)
              f += neu1[c] * syn1[c + l2];
            if (f <= -MAX_EXP)
              continue;
            else if (f >= MAX_EXP)
              continue;
            else
              f = expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))];
            // 'g' is the gradient multiplied by the learning rate
            g = (static_cast<float>(1 - vocab[word].code[d]) - f) * alpha;
            // Propagate errors output -> hidden
            for (c = 0; c < layer1_size; c++)
              neu1e[c] += g * syn1[c + l2];
            // Learn weights hidden -> output
            for (c = 0; c < layer1_size; c++)
              syn1[c + l2] += g * neu1[c];
          }

        // CBOW NEGATIVE SAMPLING
        if (negative > 0)
          for (d = 0; d < negative + 1; d++)
          {
            if (d == 0)
            {
              target = word;
              label  = 1;
            }
            else
            {
              next_random = next_random * (unsigned long long)25214903917 + 11;
              target      = table[(next_random >> 16) % table_size];
              if (target == 0)
                target = static_cast<long long>(
                    next_random % static_cast<unsigned long long>(vocab_size - 1) + 1);
              if (target == word)
                continue;
              label = 0;
            }
            l2 = target * layer1_size;
            f  = 0;
            for (c = 0; c < layer1_size; c++)
              f += neu1[c] * syn1neg[c + l2];
            if (f > MAX_EXP)
              g = static_cast<float>(label - 1) * alpha;
            else if (f < -MAX_EXP)
              g = static_cast<float>(label - 0) * alpha;
            else
              g = (label - static_cast<float>(expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))])) * alpha;
            for (c = 0; c < layer1_size; c++)
              neu1e[c] += g * syn1neg[c + l2];
            for (c = 0; c < layer1_size; c++)
              syn1neg[c + l2] += g * neu1[c];
          }

        // hidden -> in
        for (a = b; a < window * 2 + 1 - b; a++)
          if (a != window)
          {
            c = sentence_position - window + a;
            if (c < 0)
              continue;
            if (c >= sentence_length)
              continue;
            last_word = sen[c];
            if (last_word == -1)
              continue;
            for (c = 0; c < layer1_size; c++)
              syn0[c + last_word * layer1_size] += neu1e[c];
          }
      }

      // SKIP-GRAM
    }
    else
    {
      // loop over offsets within dynamic window
      // (relative to max window size)
      for (a = b; a < window * 2 + 1 - b; a++)
        if (a != window)
        {
          // compute position in sentence of input word
          // (output word pos - max window size + rel offset)
          c = sentence_position - window + a;
          // skip if input word position OOB
          // (note this is our main constraint on word position w.r.t. sentence
          // bounds)
          if (c < 0)
            continue;
          if (c >= sentence_length)
            continue;
          // compute input word index
          last_word = sen[c];
          // skip OOV (TODO checked already, should never fire)
          if (last_word == -1)
            continue;
          // compute input word row offset
          l1 = last_word * layer1_size;
          // initialize gradient for input word (work space)
          for (c = 0; c < layer1_size; c++)
            neu1e[c] = 0;

          // SKIP-GRAM HIERARCHICAL SOFTMAX
          if (hs)
            for (d = 0; d < vocab[word].codelen; d++)
            {
              f  = 0;
              l2 = vocab[word].point[d] * layer1_size;
              // Propagate hidden -> output
              for (c = 0; c < layer1_size; c++)
                f += syn0[c + l1] * syn1[c + l2];
              if (f <= -MAX_EXP)
                continue;
              else if (f >= MAX_EXP)
                continue;
              else
                f = expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))];
              // 'g' is the gradient multiplied by the learning rate
              g = (1 - vocab[word].code[d] - f) * alpha;
              // Propagate errors output -> hidden
              for (c = 0; c < layer1_size; c++)
                neu1e[c] += g * syn1[c + l2];
              // Learn weights hidden -> output
              for (c = 0; c < layer1_size; c++)
                syn1[c + l2] += g * syn0[c + l1];
            }

          // SKIP-GRAM NEGATIVE SAMPLING
          if (negative > 0)
            for (d = 0; d < negative + 1; d++)
            {
              if (d == 0)
              {
                // fetch output word
                target = word;
                label  = 1;
              }
              else
              {
                // fetch negative-sampled word
                next_random = next_random * (unsigned long long)25214903917 + 11;
                target      = table[(next_random >> 16) % table_size];
                if (target == 0)
                  target = static_cast<long long>(
                      next_random % static_cast<unsigned long long>(vocab_size - 1) + 1);
                if (target == word)
                  continue;
                label = 0;
              }
              l2 = target * layer1_size;  // output/neg-sample word row offset
              // compute f = < v_{w_I}', v_{w_O} >
              // (inner product for neg sample)
              f = 0;
              for (c = 0; c < layer1_size; c++)
                f += syn0[c + l1] * syn1neg[c + l2];
              // compute gradient coeff g = alpha * (label - 1 / (e^-f + 1))
              // (alpha is learning rate, label is 1 for output and 0 for neg)
              if (f > MAX_EXP)
                g = (label - 1) * alpha;
              else if (f < -MAX_EXP)
                g = (label - 0) * alpha;
              else
                g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) *
                    alpha;
              // contribute to gradient for input word
              for (c = 0; c < layer1_size; c++)
                neu1e[c] += g * syn1neg[c + l2];
              // perform gradient step for output/neg-sample word
              for (c = 0; c < layer1_size; c++)
                syn1neg[c + l2] += g * syn0[c + l1];
            }

          // now that we've taken gradient step for output and all neg sample
          // words, take gradient step for input word
          for (c = 0; c < layer1_size; c++)
            syn0[c + l1] += neu1e[c];
        }
    }

    // update to next output word; if we are at end of sentence, signal
    // to read new sentence
    sentence_position++;
    if (sentence_position >= sentence_length)
    {
      sentence_length = 0;
      continue;
    }
  }

  // clean up
  fclose(fi);
  free(neu1);
  free(neu1e);
  pthread_exit(NULL);
}

// Train word embeddings on text in `train_file` using one or more
// threads, either learning vocabulary from that training data (in a
// separate pass over the data) or loading the vocabulary from a file
// `read_vocab_file`.  Optionally save vocabulary to a file
// `save_vocab_file`; either save word embeddings to a file
// `output_file` or, if `classes` is greater than zero, run k-means
// clustering and save those clusters to `output_file`.
//
// If `output_file` is empty (first byte is null), do not train; this
// can be used to learn the vocabulary only from a training text file.
void TrainModel()
{
  long       a, b, c, d;  // loop counters among other things
  FILE *     fo;          // output file
  pthread_t *pt = (pthread_t *)malloc(static_cast<unsigned long>(num_threads) * sizeof(pthread_t));

  printf("Starting training using file %s\n", train_file);

  // initialize learning rate
  starting_alpha = alpha;

  // read vocab from file or learn from training data
  if (read_vocab_file[0] != 0)
    ReadVocab();
  else
    LearnVocabFromTrainFile();
  // save vocab to file
  if (save_vocab_file[0] != 0)
    SaveVocab();
  // if no `output_file` is specified, exit (do not train)
  if (output_file[0] == 0)
    return;

  // initialize network parameters
  InitNet();
  // initialize negative sampling distribution
  if (negative > 0)
    InitUnigramTable();

  start = clock();
  for (a = 0; a < num_threads; a++)
    pthread_create(&pt[a], NULL, TrainModelThread, (void *)a);
  for (a = 0; a < num_threads; a++)
    pthread_join(pt[a], NULL);
  fo = fopen(output_file, "wb");
  if (classes == 0)
  {
    // Save the word vectors
    fprintf(fo, "%lld %lld\n", vocab_size, layer1_size);
    for (a = 0; a < vocab_size; a++)
    {
      fprintf(fo, "%s ", vocab[a].word);
      if (binary)
        for (b = 0; b < layer1_size; b++)
          fwrite(&syn0[a * layer1_size + b], sizeof(real), 1, fo);
      else
        for (b = 0; b < layer1_size; b++)
          fprintf(fo, "%lf ", syn0[a * layer1_size + b]);
      fprintf(fo, "\n");
    }
  }
  else
  {
    // Run K-means on the word vectors
    int   clcn = static_cast<int>(classes), iter = 10, closeid;
    int * centcn = (int *)malloc(static_cast<unsigned long>(classes) * sizeof(int));
    int * cl     = (int *)calloc(static_cast<unsigned long>(vocab_size), sizeof(int));
    real  closev, x;
    real *cent = (real *)calloc(static_cast<unsigned long>(classes * layer1_size), sizeof(real));
    for (a = 0; a < vocab_size; a++)
      cl[a] = a % clcn;
    for (a = 0; a < iter; a++)
    {
      for (b = 0; b < clcn * layer1_size; b++)
        cent[b] = 0;
      for (b = 0; b < clcn; b++)
        centcn[b] = 1;
      for (c = 0; c < vocab_size; c++)
      {
        for (d = 0; d < layer1_size; d++)
          cent[layer1_size * cl[c] + d] += syn0[c * layer1_size + d];
        centcn[cl[c]]++;
      }
      for (b = 0; b < clcn; b++)
      {
        closev = 0;
        for (c = 0; c < layer1_size; c++)
        {
          cent[layer1_size * b + c] /= centcn[b];
          closev += cent[layer1_size * b + c] * cent[layer1_size * b + c];
        }
        closev = sqrt(closev);
        for (c = 0; c < layer1_size; c++)
          cent[layer1_size * b + c] /= closev;
      }
      for (c = 0; c < vocab_size; c++)
      {
        closev  = -10;
        closeid = 0;
        for (d = 0; d < clcn; d++)
        {
          x = 0;
          for (b = 0; b < layer1_size; b++)
            x += cent[layer1_size * d + b] * syn0[c * layer1_size + b];
          if (x > closev)
          {
            closev  = x;
            closeid = static_cast<int>(d);
          }
        }
        cl[c] = closeid;
      }
    }
    // Save the K-means classes
    for (a = 0; a < vocab_size; a++)
      fprintf(fo, "%s %d\n", vocab[a].word, cl[a]);
    free(centcn);
    free(cent);
    free(cl);
  }
  fclose(fo);
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
  int i;
  if (argc == 1)
  {
    printf("WORD VECTOR estimation toolkit v 0.1c\n\n");
    printf("Options:\n");
    printf("Parameters for training:\n");
    printf("\t-train <file>\n");
    printf("\t\tUse text data from <file> to train the model\n");
    printf("\t-output <file>\n");
    printf("\t\tUse <file> to save the resulting word vectors / word clusters\n");
    printf("\t-size <int>\n");
    printf("\t\tSet size of word vectors; default is 100\n");
    printf("\t-window <int>\n");
    printf("\t\tSet max skip length between words; default is 5\n");
    printf("\t-sample <float>\n");
    printf(
        "\t\tSet threshold for occurrence of words. Those that appear with higher frequency in the "
        "training data\n");
    printf("\t\twill be randomly down-sampled; default is 1e-3, useful range is (0, 1e-5)\n");
    printf("\t-hs <int>\n");
    printf("\t\tUse Hierarchical Softmax; default is 0 (not used)\n");
    printf("\t-negative <int>\n");
    printf(
        "\t\tNumber of negative examples; default is 5, common values are 3 - 10 (0 = not used)\n");
    printf("\t-threads <int>\n");
    printf("\t\tUse <int> threads (default 12)\n");
    printf("\t-iter <int>\n");
    printf("\t\tRun more training iterations (default 5)\n");
    printf("\t-min-count <int>\n");
    printf("\t\tThis will discard words that appear less than <int> times; default is 5\n");
    printf("\t-alpha <float>\n");
    printf(
        "\t\tSet the starting learning rate; default is 0.025 for skip-gram and 0.05 for CBOW\n");
    printf("\t-classes <int>\n");
    printf(
        "\t\tOutput word classes rather than word vectors; default number of classes is 0 (vectors "
        "are written)\n");
    printf("\t-debug <int>\n");
    printf("\t\tSet the debug mode (default = 2 = more info during training)\n");
    printf("\t-binary <int>\n");
    printf("\t\tSave the resulting vectors in binary moded; default is 0 (off)\n");
    printf("\t-save-vocab <file>\n");
    printf("\t\tThe vocabulary will be saved to <file>\n");
    printf("\t-read-vocab <file>\n");
    printf("\t\tThe vocabulary will be read from <file>, not constructed from the training data\n");
    printf("\t-cbow <int>\n");
    printf("\t\tUse the continuous bag of words model; default is 1 (use 0 for skip-gram model)\n");
    printf("\nExamples:\n");
    printf(
        "./word2vec -train data.txt -output vec.txt -size 200 -window 5 -sample 1e-4 -negative 5 "
        "-hs 0 -binary 0 -cbow 1 -iter 3\n\n");
    return 0;
  }
  output_file[0]     = 0;
  save_vocab_file[0] = 0;
  read_vocab_file[0] = 0;
  if ((i = ArgPos((char *)"-size", argc, argv)) > 0)
    layer1_size = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-train", argc, argv)) > 0)
    strcpy(train_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-save-vocab", argc, argv)) > 0)
    strcpy(save_vocab_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-read-vocab", argc, argv)) > 0)
    strcpy(read_vocab_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-debug", argc, argv)) > 0)
    debug_mode = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-binary", argc, argv)) > 0)
    binary = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-cbow", argc, argv)) > 0)
    cbow = atoi(argv[i + 1]);
  if (cbow)
    alpha = static_cast<float>(0.05);
  if ((i = ArgPos((char *)"-alpha", argc, argv)) > 0)
    alpha = static_cast<float>(atof(argv[i + 1]));
  if ((i = ArgPos((char *)"-output", argc, argv)) > 0)
    strcpy(output_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-window", argc, argv)) > 0)
    window = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-sample", argc, argv)) > 0)
    sample = static_cast<float>(atof(argv[i + 1]));
  if ((i = ArgPos((char *)"-hs", argc, argv)) > 0)
    hs = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-negative", argc, argv)) > 0)
    negative = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-threads", argc, argv)) > 0)
    num_threads = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-iter", argc, argv)) > 0)
    iter = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-min-count", argc, argv)) > 0)
    min_count = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-classes", argc, argv)) > 0)
    classes = atoi(argv[i + 1]);
  vocab      = (struct vocab_word *)calloc(static_cast<unsigned long>(vocab_max_size),
                                      sizeof(struct vocab_word));
  vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
  // precompute e^x / (e^x + 1) for x in [-MAX_EXP, MAX_EXP)
  // TODO extra element (+ 1) seems unused?
  expTable = (real *)malloc((EXP_TABLE_SIZE + 1) * sizeof(real));
  for (i = 0; i < EXP_TABLE_SIZE; i++)
  {
    // Precompute the exp() table
    expTable[i] = exp((i / (real)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP);
    // Precompute f(x) = x / (x + 1)
    expTable[i] = expTable[i] / (expTable[i] + 1);
  }
  TrainModel();
  return 0;
}
