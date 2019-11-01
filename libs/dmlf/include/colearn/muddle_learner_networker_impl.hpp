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
  
class MuddleLearnerNetworkerImpl
{
public:
  MuddleLearnerNetworkerImpl()
  {
  }
  virtual ~MuddleLearnerNetworkerImpl()
  {
  }

  MuddleLearnerNetworkerImpl(MuddleLearnerNetworkerImpl const &other) = delete; // { copy(other); }
  MuddleLearnerNetworkerImpl &operator=(MuddleLearnerNetworkerImpl const &other) = delete; // { copy(other); return *this; }
  bool operator==(MuddleLearnerNetworkerImpl const &other) = delete; // const { return compare(other)==0; }
  bool operator<(MuddleLearnerNetworkerImpl const &other) = delete; // const { return compare(other)==-1; }

  //friend std::ostream& operator<<(std::ostream& os, MuddleLearnerNetworkerImpl const &output);
  //friend void swap(MuddleLearnerNetworkerImpl &a, MuddleLearnerNetworkerImpl &b);
protected:
  // int compare(const MuddleLearnerNetworkerImpl &other) const { ... }
  // void copy(const MuddleLearnerNetworkerImpl &other) { ... }
  // void clear(void) { ... }
  // bool empty(void) const { ... }
  // void swap(MuddleLearnerNetworkerImpl &other) { ... }
private:

  //bool operator!=(MuddleLearnerNetworkerImpl const &other) const { return compare(other)!=0; }
  //bool operator>(MuddleLearnerNetworkerImpl const &other) const { return compare(other)==1; }
  //bool operator<=(MuddleLearnerNetworkerImpl const &other) const { return compare(other)!=1; }
  //bool operator>=(MuddleLearnerNetworkerImpl const &other) const { return compare(other)!=-1; }
};

}
}
//std::ostream& operator<<(std::ostream& os, const MuddleLearnerNetworkerImpl &output) {}
//void swap(MuddleLearnerNetworkerImpl& v1, MuddleLearnerNetworkerImpl& v2);
