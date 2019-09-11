#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class SimpleCyclingAlgorithm
{
public:
  SimpleCyclingAlgorithm()
  {
  }
  virtual ~SimpleCyclingAlgorithm()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const SimpleCyclingAlgorithm &output);
  //friend void swap(SimpleCyclingAlgorithm &a, SimpleCyclingAlgorithm &b);
protected:
  // int compare(const SimpleCyclingAlgorithm &other) const { ... }
  // void copy(const SimpleCyclingAlgorithm &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(SimpleCyclingAlgorithm &other) { ... }
private:
  SimpleCyclingAlgorithm(const SimpleCyclingAlgorithm &other) = delete; // { copy(other); }
  SimpleCyclingAlgorithm &operator=(const SimpleCyclingAlgorithm &other) = delete; // { copy(other); return *this; }
  bool operator==(const SimpleCyclingAlgorithm &other) = delete; // const { return compare(other)==0; }
  bool operator<(const SimpleCyclingAlgorithm &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const SimpleCyclingAlgorithm &other) const { return compare(other)!=0; }
  //bool operator>(const SimpleCyclingAlgorithm &other) const { return compare(other)==1; }
  //bool operator<=(const SimpleCyclingAlgorithm &other) const { return compare(other)!=1; }
  //bool operator>=(const SimpleCyclingAlgorithm &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const SimpleCyclingAlgorithm &output) {}
//void swap(SimpleCyclingAlgorithm& v1, SimpleCyclingAlgorithm& v2);
