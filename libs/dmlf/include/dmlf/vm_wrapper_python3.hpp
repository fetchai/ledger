#pragma once

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class vm_wrapper_python3
{
public:
  vm_wrapper_python3()
  {
  }
  virtual ~vm_wrapper_python3()
  {
  }

  //friend std::ostream& operator<<(std::ostream& os, const vm_wrapper_python3 &output);
  //friend void swap(vm_wrapper_python3 &a, vm_wrapper_python3 &b);
protected:
  // int compare(const vm_wrapper_python3 &other) const { ... }
  // void copy(const vm_wrapper_python3 &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(vm_wrapper_python3 &other) { ... }
private:
  vm_wrapper_python3(const vm_wrapper_python3 &other) = delete; // { copy(other); }
  vm_wrapper_python3 &operator=(const vm_wrapper_python3 &other) = delete; // { copy(other); return *this; }
  bool operator==(const vm_wrapper_python3 &other) = delete; // const { return compare(other)==0; }
  bool operator<(const vm_wrapper_python3 &other) = delete; // const { return compare(other)==-1; }

  //bool operator!=(const vm_wrapper_python3 &other) const { return compare(other)!=0; }
  //bool operator>(const vm_wrapper_python3 &other) const { return compare(other)==1; }
  //bool operator<=(const vm_wrapper_python3 &other) const { return compare(other)!=1; }
  //bool operator>=(const vm_wrapper_python3 &other) const { return compare(other)!=-1; }
};

//std::ostream& operator<<(std::ostream& os, const vm_wrapper_python3 &output) {}
//void swap(vm_wrapper_python3& v1, vm_wrapper_python3& v2);
