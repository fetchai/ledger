#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class IUpdate
{
public:
  IUpdate()
  {
  }
  virtual ~IUpdate()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const IUpdate &output);
  //friend void swap(IUpdate &a, IUpdate &b);
protected:
  // int compare(const IUpdate &other) const { ... }
  // void copy(const IUpdate &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(IUpdate &other) { ... }
private:
  IUpdate(const IUpdate &other) = delete; // { copy(other); }
  IUpdate &operator=(const IUpdate &other) = delete; // { copy(other); return *this; }
  bool operator==(const IUpdate &other) = delete; // const { return compare(other)==0; }
  bool operator<(const IUpdate &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const IUpdate &other) const { return compare(other)!=0; }
  //bool operator>(const IUpdate &other) const { return compare(other)==1; }
  //bool operator<=(const IUpdate &other) const { return compare(other)!=1; }
  //bool operator>=(const IUpdate &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const IUpdate &output) {}
//void swap(IUpdate& v1, IUpdate& v2);
