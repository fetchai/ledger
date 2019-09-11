#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class IShuffleAlgorithm
{
public:
  IShuffleAlgorithm()
  {
  }
  virtual ~IShuffleAlgorithm()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const IShuffleAlgorithm &output);
  //friend void swap(IShuffleAlgorithm &a, IShuffleAlgorithm &b);
protected:
  // int compare(const IShuffleAlgorithm &other) const { ... }
  // void copy(const IShuffleAlgorithm &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(IShuffleAlgorithm &other) { ... }
private:
  IShuffleAlgorithm(const IShuffleAlgorithm &other) = delete; // { copy(other); }
  IShuffleAlgorithm &operator=(const IShuffleAlgorithm &other) = delete; // { copy(other); return *this; }
  bool operator==(const IShuffleAlgorithm &other) = delete; // const { return compare(other)==0; }
  bool operator<(const IShuffleAlgorithm &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const IShuffleAlgorithm &other) const { return compare(other)!=0; }
  //bool operator>(const IShuffleAlgorithm &other) const { return compare(other)==1; }
  //bool operator<=(const IShuffleAlgorithm &other) const { return compare(other)!=1; }
  //bool operator>=(const IShuffleAlgorithm &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const IShuffleAlgorithm &output) {}
//void swap(IShuffleAlgorithm& v1, IShuffleAlgorithm& v2);
