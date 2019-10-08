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

#include "dmlf/abstract_learner_networker.hpp"
#include <memory>
#include <set>
#include <thread>

namespace fetch {
namespace dmlf {

class FilepassingLearnerNetworker : public AbstractLearnerNetworker
{
public:
  using Bytes                = AbstractLearnerNetworker::Bytes;
  using Peer                 = std::string;
  using Peers                = std::vector<Peer>;
  using ProcessedUpdateNames = std::unordered_set<std::string>;
  using ThreadP              = std::shared_ptr<std::thread>;

  FilepassingLearnerNetworker();
  ~FilepassingLearnerNetworker() override;

  void        PushUpdate(const std::shared_ptr<UpdateInterface> &update) override;
  std::size_t GetPeerCount() const override
  {
    return peers_.size();
  }

  void SetName(const std::string &name);
  void AddPeers(Peers new_peers);
  void ClearPeers();

  FilepassingLearnerNetworker(const FilepassingLearnerNetworker &other) = delete;
  FilepassingLearnerNetworker &operator=(const FilepassingLearnerNetworker &other)  = delete;
  bool                         operator==(const FilepassingLearnerNetworker &other) = delete;
  bool                         operator<(const FilepassingLearnerNetworker &other)  = delete;

protected:
  static std::string       ProcessNameToTargetDir(const std::string &name);
  void                     Transmit(const std::string &target, const Bytes &data);
  std::vector<std::string> GetUpdateNames() const;
  void                     CheckUpdates();

private:
  ProcessedUpdateNames processed_updates_;
  Peers                peers_;
  ThreadP              watcher_;

  std::string name_;
  std::string dir_;
  bool        running_;
};

}  // namespace dmlf
}  // namespace fetch
