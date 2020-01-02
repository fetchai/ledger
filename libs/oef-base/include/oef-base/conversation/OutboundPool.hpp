#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

// Delete bits as needed

//#include <algorithm>
//#include <utility>
//#include <iostream>

class OutboundPool
{
public:
  OutboundPool()
  {}
  virtual ~OutboundPool()
  {}

  // friend std::ostream& operator<<(std::ostream& os, const OutboundPool &output);
  // friend void swap(OutboundPool &a, OutboundPool &b);
protected:
  // int compare(const OutboundPool &other) const { ... }
  // void copy(const OutboundPool &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(OutboundPool &other) { ... }
private:
  OutboundPool(const OutboundPool &other) = delete;             // { copy(other); }
  OutboundPool &operator=(const OutboundPool &other) = delete;  // { copy(other); return *this; }
  bool operator==(const OutboundPool &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const OutboundPool &other)  = delete;  // const { return compare(other)==-1; }

  // bool operator!=(const OutboundPool &other) const { return compare(other)!=0; }
  // bool operator>(const OutboundPool &other) const { return compare(other)==1; }
  // bool operator<=(const OutboundPool &other) const { return compare(other)!=1; }
  // bool operator>=(const OutboundPool &other) const { return compare(other)!=-1; }
};

// namespace std { template<> void swap(OutboundPool& lhs, OutboundPool& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const OutboundPool &output) {}
