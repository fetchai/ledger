#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class MuddleLearnerNetworker
{
public:
  MuddleLearnerNetworker()
  {
  }
  virtual ~MuddleLearnerNetworker()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const MuddleLearnerNetworker &output);
  //friend void swap(MuddleLearnerNetworker &a, MuddleLearnerNetworker &b);
protected:
  // int compare(const MuddleLearnerNetworker &other) const { ... }
  // void copy(const MuddleLearnerNetworker &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(MuddleLearnerNetworker &other) { ... }
private:
  MuddleLearnerNetworker(const MuddleLearnerNetworker &other) = delete; // { copy(other); }
  MuddleLearnerNetworker &operator=(const MuddleLearnerNetworker &other) = delete; // { copy(other); return *this; }
  bool operator==(const MuddleLearnerNetworker &other) = delete; // const { return compare(other)==0; }
  bool operator<(const MuddleLearnerNetworker &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const MuddleLearnerNetworker &other) const { return compare(other)!=0; }
  //bool operator>(const MuddleLearnerNetworker &other) const { return compare(other)==1; }
  //bool operator<=(const MuddleLearnerNetworker &other) const { return compare(other)!=1; }
  //bool operator>=(const MuddleLearnerNetworker &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const MuddleLearnerNetworker &output) {}
//void swap(MuddleLearnerNetworker& v1, MuddleLearnerNetworker& v2);
