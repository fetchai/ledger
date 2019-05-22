#include "math/tensor.hpp"
#include <fstream>
#include <string.h>

using DataType  = double;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

template <typename ArrayType>
class DataLoader
{
public:
  using IteratorType = typename ArrayType::IteratorType;

  std::vector<std::string> word_order{};

  ///
  /// Data & cursors
  ///

  ArrayType    data_;
  SizeType     cursor_offset_;
  SizeType     n_positive_cursors_;
  IteratorType cursor_ = {data_};

  // positive cursors
  std::vector<IteratorType> positive_cursors_;

  ///
  /// Random values
  ///

  fetch::random::LinearCongruentialGenerator gen;
  SizeType                                   ran_val_;
  //  SizeType const                             N_RANDOM_ROWS = 1000000;
  std::vector<SizeType> ran_positive_cursor_{1000000};
  std::vector<SizeType> rows_{};

  SizeType max_sentence_len_ = 0;
  SizeType min_word_freq_;
  SizeType max_word_len_;

  std::unordered_map<std::string, SizeType> vocab_;              // unique vocab of words
  std::unordered_map<SizeType, SizeType>    vocab_frequencies_;  // the count of each vocab word

  DataLoader(SizeType max_sentence_len, SizeType min_word_freq, SizeType max_sentences,
             SizeType window_size, SizeType max_word_len)
    : data_({max_sentence_len, max_sentences})
    , cursor_offset_(window_size)
    , n_positive_cursors_(2 * window_size)
    , max_sentence_len_(max_sentence_len)
    , min_word_freq_(min_word_freq)
    , max_word_len_(max_word_len)
  {
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      positive_cursors_.emplace_back(IteratorType(data_));
    }

    PrepareDynamicWindowProbs();
  }

  /**
   * Read a single text file
   * @param path
   * @return
   */
  static std::string ReadFile(std::string const &path)
  {
    std::ifstream t(path);
    return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  }

  void AddData(std::string const &filename)
  {
    std::string text = ReadFile(filename);

    cursor_ = data_.begin();

    // replace all tabs with spaces.
    std::replace(text.begin(), text.end(), '\t', ' ');

    // replace all new lines with spaces.
    std::replace(text.begin(), text.end(), '\n', ' ');

    // replace carriage returns with spaces
    std::replace(text.begin(), text.end(), '\r', ' ');

    vocab_.emplace(std::make_pair("UNK", 0));
    vocab_frequencies_.emplace(std::make_pair(0, 0));

    // initial iteration through words
    SizeType          word_idx;
    std::string       word;
    std::stringstream s(text);
    for (; s >> word;)
    {
      if (word.length() >= max_word_len_ - 1)
      {
        word = word.substr(0, max_word_len_ - 2);
      }

      word_order.emplace_back(word);

      if (!(cursor_.is_valid()))
      {
        break;
      }

      // if already seen this word
      auto it = vocab_.find(word);
      if (it != vocab_.end())
      {
        vocab_frequencies_.at(it->second)++;
        word_idx = it->second;
      }
      else
      {
        word_idx = vocab_.size();
        vocab_frequencies_.emplace(std::make_pair(vocab_.size(), SizeType(1)));
        vocab_.emplace(std::make_pair(word, vocab_.size()));
      }

      // write the word index to the data tensor
      *cursor_ = static_cast<DataType>(word_idx);
      ++cursor_;
    }

    // prune infrequent words
    PruneVocab();

    // second iteration to assign data values after pruning
    s                     = std::stringstream(text);
    cursor_               = data_.begin();
    SizeType cursor_count = 0;
    for (; s >> word;)
    {
      if (word.length() >= max_word_len_ - 1)
      {
        word = word.substr(0, max_word_len_ - 2);
      }

      if (!(cursor_.is_valid()))
      {
        break;
      }

      // if the word is in the vocab assign it, otherwise assign 0
      auto it = vocab_.find(word);
      if (it != vocab_.end())
      {
        *cursor_ = static_cast<DataType>(it->second);
      }
      else
      {
        *cursor_ = DataType(0);
      }
      ++cursor_;
      ++cursor_count;
    }

    // guarantee that data_ is filled with zeroes after the last word added

    // remove uneccesary data rows
    if (cursor_count < data_.size())
    {
      SizeType remaining_idxs = (data_.size() - cursor_count);
      // if there are unused rows
      if (remaining_idxs > max_sentence_len_)
      {
        SizeType redundant_rows = remaining_idxs / max_sentence_len_;
        data_.Resize({data_.shape()[0], data_.shape()[1] - redundant_rows}, true);
      }
    }

    // ensure final row has zeroes where necessary
    for (std::size_t i = cursor_count; i < data_.size(); ++i)
    {
      assert(data_[i] == 0);
    }

    // reset the cursor
    reset_cursor();

    // build the unigram table for negative sampling
    BuildUnigramTable();

    std::cout << "vocab size: " << vocab_size() << std::endl;
    std::cout << "words in train file: " << cursor_count << std::endl;
  }

  // prune words that are infrequent & sort the words
  void PruneVocab()
  {
    // copy existing vocabs into temporary storage
    std::unordered_map<std::string, SizeType> tmp_vocab              = vocab_;
    std::unordered_map<SizeType, SizeType>    tmp_vocab_frequencies_ = vocab_frequencies_;

    // clear existing vocabs
    vocab_.clear();
    vocab_frequencies_.clear();

    vocab_.emplace(std::make_pair("UNK", 0));
    vocab_frequencies_.emplace(std::make_pair(0, 0));

    // Get pruned idxs
    for (auto &word : tmp_vocab)
    {
      auto cur_freq = tmp_vocab_frequencies_.at(word.second);
      if (cur_freq >= min_word_freq_)
      {
        // don't prune
        vocab_.emplace(std::make_pair(word.first, vocab_.size()));
        vocab_frequencies_.emplace(std::make_pair(vocab_.size(), cur_freq));
      }
    }
  }

  SizeType vocab_size()
  {
    return vocab_.size();
  }

  SizeType vocab_lookup(std::string &word)
  {
    return vocab_[word];
  }

  std::string VocabLookup(SizeType word_idx)
  {
    auto vocab_it = vocab_.begin();
    while (vocab_it != vocab_.end())
    {
      if (vocab_it->second == word_idx)
      {
        return vocab_it->first;
      }
      ++vocab_it;
    }
    return "UNK";
  }

  void next_positive(SizeType &input_idx, SizeType &context_idx)
  {
    input_idx = static_cast<SizeType>(*cursor_);

    // generate random value from 0 -> 2xwindow_size
    ran_val_ = gen() % ran_positive_cursor_.size();

    // dynamic context window - pick positive cursor
    context_idx = static_cast<SizeType>(*(positive_cursors_[ran_positive_cursor_[ran_val_]]));

    assert(input_idx < vocab_.size());
  }

  void next_negative(SizeType &input_idx, SizeType &context_idx)
  {
    input_idx = static_cast<SizeType>(*cursor_);

    //    // randomly select a negative cursor from all words in vocab
    //    // TODO: strictly we should ensure this is not one of the positive context words
    //    context_idx = static_cast<SizeType>(gen() % vocab_.size());

    // randomly select an index from the unigram table

    auto ran_val = static_cast<SizeType>(gen() % unigram_size_);

    context_idx = unigram_table_[ran_val];

    assert(context_idx < vocab_size());
  }

  void IncrementCursors()
  {
    ++cursor_;
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      ++(positive_cursors_[i]);
    }
  }

  bool done()
  {
    // we just check that the final positive window cursor isn't valid
    return (!(positive_cursors_[n_positive_cursors_ - 1].is_valid()));
  }

  void reset_cursor()
  {
    // the data cursor is on the current index
    cursor_ = IteratorType(data_, cursor_offset_);

    // set up the positive cursors and shift them
    for (std::size_t j = 0; j < 2 * cursor_offset_; ++j)
    {
      if (j < cursor_offset_)
      {
        positive_cursors_[j] = IteratorType(data_, cursor_offset_ - (cursor_offset_ - j));
      }
      else
      {
        positive_cursors_[j] = IteratorType(data_, cursor_offset_ + (j - cursor_offset_ + 1));
      }
    }

    ASSERT(cursor_.is_valid());
    for (std::size_t l = 0; l < n_positive_cursors_; ++l)
    {
      ASSERT(positive_cursors_[l].is_valid());
    }
  }

private:
  static constexpr SizeType unigram_size_ = 1000000;
  SizeType                  unigram_table_[unigram_size_];
  double                    unigram_power_ = 0.75;

  void PrepareDynamicWindowProbs()
  {
    // get sum of frequencies for dynamic window
    SizeType sum_freqs = 0;
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      if (i < cursor_offset_)
      {
        sum_freqs += i + 1;
      }
      else
      {
        sum_freqs += (cursor_offset_ - (i - cursor_offset_));
      }
    }

    // calculate n rows per cursor
    for (std::size_t i = 0; i < n_positive_cursors_; ++i)
    {
      if (i < cursor_offset_)
      {
        for (std::size_t j = 0;
             j < static_cast<std::size_t>(
                     (static_cast<double>(i + 1) / static_cast<double>(sum_freqs)) *
                     static_cast<double>(ran_positive_cursor_.size()));
             ++j)
        {
          rows_.emplace_back(i);
        }
      }
      else
      {
        for (std::size_t j = 0;
             j < static_cast<std::size_t>(
                     (static_cast<double>((cursor_offset_ - (i - cursor_offset_))) /
                      static_cast<double>(sum_freqs)) *
                     static_cast<double>(ran_positive_cursor_.size()));
             ++j)
        {
          rows_.emplace_back(i);
        }
      }
    }

    // move from vector into fixed array
    for (std::size_t k = 0; k < ran_positive_cursor_.size(); ++k)
    {
      if (k < rows_.size())
      {
        ran_positive_cursor_[k] = rows_[k];
      }

      // TODO - this probably doesn't work as expected
      // if the vector and array are different sizes, just fill up the extra entries with high
      // probability cases
      else
      {
        ran_positive_cursor_[k] = cursor_offset_;
      }
    }
  }

  void BuildUnigramTable()
  {
    vocab_.size();
    double train_words_pow = 0.;
    double d1              = 0.;

    // compute normalizer
    for (SizeType a = 0; a < vocab_.size(); ++a)
    {
      train_words_pow += pow(static_cast<double>(vocab_frequencies_[a]), unigram_power_);
    }

    //
    SizeType word_idx = 0;
    d1 = pow(static_cast<double>(vocab_frequencies_[word_idx]), unigram_power_) / train_words_pow;

    auto vocab_it = vocab_.begin();

    for (SizeType a = 0; a < unigram_size_; a++)
    {
      unigram_table_[a] = vocab_it->second;
      if ((static_cast<double>(a) / static_cast<double>(unigram_size_)) > d1)
      {
        if (vocab_it != vocab_.end())
        {
          ++vocab_it;
        }
        d1 += pow(static_cast<double>(vocab_frequencies_[vocab_it->second]), unigram_power_) /
              train_words_pow;
      }
    }
  }
};

class OrigW2vDataLoader
{
public:
#define MAX_STRING 100
#define MAX_CODE_LENGTH 40

  // Maximum 30 * 0.7 = 21M words in the vocabulary
  // (where 0.7 is the magical load factor beyond which hash table
  // performance degrades)
  const int vocab_hash_size = 30000000;

  char *    train_file;
  int *     vocab_hash;
  long long file_size      = 0;
  long long vocab_size     = 0;
  long long train_words    = 0;
  long long vocab_max_size = 1000;
  int       min_count;

  std::vector<std::string> word_order;

  OrigW2vDataLoader(std::string filename, int min_freq)
  {

    vocab      = (struct vocab_word *)calloc(static_cast<unsigned long>(vocab_max_size),
                                        sizeof(struct vocab_word));
    vocab_hash = (int *)calloc(static_cast<size_t>(vocab_hash_size), sizeof(int));

    train_file = new char[filename.length() + 1];
    std::strcpy(train_file, filename.c_str());
    min_count = min_freq;
  }

  // Representation of a word in the vocabulary, including (optional,
  // for hierarchical softmax only) Huffman coding
  struct vocab_word
  {
    long long cn;
    int *     point;
    char *    word, *code, codelen;
  };

  struct vocab_word *vocab;  // vocabulary

  // Return hash (integer between 0, inclusive, and `vocab_hash_size`,
  // exclusive) of `word`
  int GetWordHash(char *word)
  {
    unsigned long long a, hash = 0;
    for (a = 0; a < strlen(word); a++)
      hash = hash * 257 + static_cast<unsigned long long>(word[a]);
    hash = hash % static_cast<unsigned long long>(vocab_hash_size);
    return static_cast<int>(hash);
  }

  // Given pointers `a` and `b` to `vocab_word` structs, return 1 if the
  // stored count `cn` of b is greater than that of a, -1 if less than,
  // and 0 if equal.  (Comparator for a reverse sort by word count.)
  static int VocabCompare(const void *a, const void *b)
  {
    long long l = ((struct vocab_word *)b)->cn - ((struct vocab_word *)a)->cn;
    if (l > 0)
      return 1;
    if (l < 0)
      return -1;
    return 0;
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
      hash = (hash + 1) % static_cast<unsigned int>(vocab_hash_size);
    }
    return -1;
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
      hash = (hash + 1) % static_cast<unsigned int>(vocab_hash_size);
    vocab_hash[hash] = static_cast<int>(vocab_size - 1);
    return static_cast<int>(vocab_size - 1);
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
          hash = (hash + 1) % static_cast<unsigned int>(vocab_hash_size);
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

    AddWordToVocab((char *)"</s>");
    while (1)
    {
      // TODO: if file is not newline-terminated, last word may be
      // swallowed
      ReadWord(word, fin, &eof);

      word_order.emplace_back(std::string(word));

      if (eof)
        break;
      train_words++;
      wc++;
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
    }
    SortVocab();

    printf("Vocab size: %lld\n", vocab_size);
    printf("Words in train file: %lld\n", train_words);

    file_size = ftell(fin);
    fclose(fin);
  }
};

int main()
{

  std::string filename = "/Users/khan/fetch/corpora/text8";

  SizeType max_sentences    = 10010;  // maximum sentences for dataloader
  SizeType max_sentence_len = 1700;   // maximum sentence length for dataloader
  SizeType min_word_freq    = 5;      // infrequent word are pruned
  SizeType window_size      = 8;      // one side of the context window
  SizeType max_word_len     = 100;    // no word should be longer than this

  // my vocab building
  DataLoader<ArrayType> my_dl(max_sentence_len, min_word_freq, max_sentences, window_size,
                              max_word_len);
  my_dl.AddData(filename);

  // w2v orig vocab building
  OrigW2vDataLoader orig_dl(filename, static_cast<int>(min_word_freq));
  orig_dl.LearnVocabFromTrainFile();

  // vocab comparison
  std::cout << "my_dl.word_order.size(): " << my_dl.word_order.size() << std::endl;
  std::cout << "orig_dl.word_order.size(): " << orig_dl.word_order.size() << std::endl;

  for (std::size_t i = 0; i < orig_dl.word_order.size(); ++i)
  {
    if (my_dl.word_order[i] != orig_dl.word_order[i])
    {
      std::cout << "different words: " << my_dl.word_order[i] << " vs " << orig_dl.word_order[i]
                << std::endl;
    }
  }
}