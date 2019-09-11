#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class Update
{
public:
  Update()
  {
  }
  virtual ~Update()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const Update &output);
  //friend void swap(Update &a, Update &b);
protected:
  // int compare(const Update &other) const { ... }
  // void copy(const Update &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(Update &other) { ... }
private:
  Update(const Update &other) = delete; // { copy(other); }
  Update &operator=(const Update &other) = delete; // { copy(other); return *this; }
  bool operator==(const Update &other) = delete; // const { return compare(other)==0; }
  bool operator<(const Update &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const Update &other) const { return compare(other)!=0; }
  //bool operator>(const Update &other) const { return compare(other)==1; }
  //bool operator<=(const Update &other) const { return compare(other)!=1; }
  //bool operator>=(const Update &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const Update &output) {}
//void swap(Update& v1, Update& v2);
