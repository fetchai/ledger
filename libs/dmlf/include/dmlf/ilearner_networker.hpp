#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class ILearnerNetworker
{
public:
  ILearnerNetworker()
  {
  }
  virtual ~ILearnerNetworker()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const ILearnerNetworker &output);
  //friend void swap(ILearnerNetworker &a, ILearnerNetworker &b);
protected:
  // int compare(const ILearnerNetworker &other) const { ... }
  // void copy(const ILearnerNetworker &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(ILearnerNetworker &other) { ... }
private:
  ILearnerNetworker(const ILearnerNetworker &other) = delete; // { copy(other); }
  ILearnerNetworker &operator=(const ILearnerNetworker &other) = delete; // { copy(other); return *this; }
  bool operator==(const ILearnerNetworker &other) = delete; // const { return compare(other)==0; }
  bool operator<(const ILearnerNetworker &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const ILearnerNetworker &other) const { return compare(other)!=0; }
  //bool operator>(const ILearnerNetworker &other) const { return compare(other)==1; }
  //bool operator<=(const ILearnerNetworker &other) const { return compare(other)!=1; }
  //bool operator>=(const ILearnerNetworker &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const ILearnerNetworker &output) {}
//void swap(ILearnerNetworker& v1, ILearnerNetworker& v2);
