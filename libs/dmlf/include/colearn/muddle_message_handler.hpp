#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

namespace fetch {
namespace colearn {
  
class MuddleMessageHandler
{
public:
  MuddleMessageHandler()
  {
  }
  virtual ~MuddleMessageHandler()
  {
  }

  MuddleMessageHandler(MuddleMessageHandler const &other) = delete; // { copy(other); }
  MuddleMessageHandler &operator=(MuddleMessageHandler const &other) = delete; // { copy(other); return *this; }
  bool operator==(MuddleMessageHandler const &other) = delete; // const { return compare(other)==0; }
  bool operator<(MuddleMessageHandler const &other) = delete; // const { return compare(other)==-1; }

  //friend std::ostream& operator<<(std::ostream& os, MuddleMessageHandler const &output);
  //friend void swap(MuddleMessageHandler &a, MuddleMessageHandler &b);
protected:
  // int compare(const MuddleMessageHandler &other) const { ... }
  // void copy(const MuddleMessageHandler &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(MuddleMessageHandler &other) { ... }
private:

  //bool operator!=(MuddleMessageHandler const &other) const { return compare(other)!=0; }
  //bool operator>(MuddleMessageHandler const &other) const { return compare(other)==1; }
  //bool operator<=(MuddleMessageHandler const &other) const { return compare(other)!=1; }
  //bool operator>=(MuddleMessageHandler const &other) const { return compare(other)!=-1; }
};

}
}
//std::ostream& operator<<(std::ostream& os, const MuddleMessageHandler &output) {}
//void swap(MuddleMessageHandler& v1, MuddleMessageHandler& v2);
