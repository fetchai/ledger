#pragma once


class UnigramTable
{
public:
  UnigramTable(unsigned int size = 0, std::vector<uint64_t> const &frequencies = {})
  {    
    Reset(size, frequencies);
  }

  void Reset(unsigned int size, std::vector<uint64_t> const &frequencies)
  {
    if (size && frequencies.size())
      {
	data_.resize(size);
	double total(0);
	for (auto const &e : frequencies)
	  {
	    total += std::pow(e, 0.75);
	  }
	std::size_t i(0);
	double n = pow((double)frequencies[i], 0.75) / total;
	for (std::size_t j(0) ; j < size ; ++j)
	  {
	    data_[j] = i;
	    if ((double)j / (double)size > n)
	      {
		i++;
		n += pow((double)frequencies[i], 0.75) / total;
	      }	
	  }
	assert(i == frequencies.size() - 1);
      }
  }
  
  uint64_t Sample() const
  {
    return data_[(uint64_t)rand() % data_.size()];
  }
  
private:
  std::vector<uint64_t> data_;
};
