#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class LocalLearnerNetworker
{
public:
  LocalLearnerNetworker()
  {
  }
  virtual ~LocalLearnerNetworker()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const LocalLearnerNetworker &output);
  //friend void swap(LocalLearnerNetworker &a, LocalLearnerNetworker &b);
protected:
  // int compare(const LocalLearnerNetworker &other) const { ... }
  // void copy(const LocalLearnerNetworker &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(LocalLearnerNetworker &other) { ... }
private:
  LocalLearnerNetworker(const LocalLearnerNetworker &other) = delete; // { copy(other); }
  LocalLearnerNetworker &operator=(const LocalLearnerNetworker &other) = delete; // { copy(other); return *this; }
  bool operator==(const LocalLearnerNetworker &other) = delete; // const { return compare(other)==0; }
  bool operator<(const LocalLearnerNetworker &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const LocalLearnerNetworker &other) const { return compare(other)!=0; }
  //bool operator>(const LocalLearnerNetworker &other) const { return compare(other)==1; }
  //bool operator<=(const LocalLearnerNetworker &other) const { return compare(other)!=1; }
  //bool operator>=(const LocalLearnerNetworker &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const LocalLearnerNetworker &output) {}
//void swap(LocalLearnerNetworker& v1, LocalLearnerNetworker& v2);
