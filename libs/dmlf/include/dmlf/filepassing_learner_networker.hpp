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

#include "dmlf/ilearner_networker.hpp"
#include <memory>

namespace fetch {
namespace dmlf {

class FilepassingLearnerNetworker : public ILearnerNetworker
{
public:
  using Intermediate = byte_array::ByteArray;
  using Peer         = std::string;
  using Peers        = std::vector<Peer>;

  FilepassingLearnerNetworker();
  virtual ~FilepassingLearnerNetworker();

  virtual void        pushUpdate(std::shared_ptr<IUpdate> update);
  virtual std::size_t getUpdateCount() const;

  virtual std::size_t getPeerCount() const
  {
    return peers_.size();
  }

  void setName(const std::string &name);
  void addPeers(Peers new_peers);
  void clearPeers();

protected:
  virtual Intermediate     getUpdateIntermediate();
  static std::string       processNameToTargetDir(const std::string name);
  void                     tx(const std::string &target, const Intermediate &data);
  std::vector<std::string> getUpdateNames() const;

private:
  FilepassingLearnerNetworker(const FilepassingLearnerNetworker &other) = delete;
  FilepassingLearnerNetworker &operator=(const FilepassingLearnerNetworker &other)  = delete;
  bool                         operator==(const FilepassingLearnerNetworker &other) = delete;
  bool                         operator<(const FilepassingLearnerNetworker &other)  = delete;

  std::set<std::string>    processed_updates_;
  std::vector<std::string> peers_;
  std::string              name_;
  std::string              dir_;
};

}  // namespace dmlf
}  // namespace fetch
