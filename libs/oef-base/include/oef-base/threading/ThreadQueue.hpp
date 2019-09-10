#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class ThreadQueue
{
public:
  ThreadQueue()
  {
  }
  virtual ~ThreadQueue()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const ThreadQueue &output);
  //friend void swap(ThreadQueue &a, ThreadQueue &b);
protected:
  // int compare(const ThreadQueue &other) const { ... }
  // void copy(const ThreadQueue &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(ThreadQueue &other) { ... }
private:
  ThreadQueue(const ThreadQueue &other) = delete; // { copy(other); }
  ThreadQueue &operator=(const ThreadQueue &other) = delete; // { copy(other); return *this; }
  bool operator==(const ThreadQueue &other) = delete; // const { return compare(other)==0; }
  bool operator<(const ThreadQueue &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const ThreadQueue &other) const { return compare(other)!=0; }
  //bool operator>(const ThreadQueue &other) const { return compare(other)==1; }
  //bool operator<=(const ThreadQueue &other) const { return compare(other)!=1; }
  //bool operator>=(const ThreadQueue &other) const { return compare(other)!=-1; }
};

//namespace std { template<> void swap(ThreadQueue& lhs, ThreadQueue& rhs) { lhs.swap(rhs); } }
//std::ostream& operator<<(std::ostream& os, const ThreadQueue &output) {}
