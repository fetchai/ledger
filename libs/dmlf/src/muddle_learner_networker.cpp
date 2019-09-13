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

#include "dmlf/muddle_learner_networker.hpp"
#include "dmlf/update.hpp"

#include "network/service/function.hpp"

#include <random>

namespace fetch {
namespace dmlf {

uint16_t ephem_port_()
{
  uint16_t from = 50000;
  uint16_t to   = 65000;
  std::random_device                  rand_dev;
  std::mt19937                        generator(rand_dev());
  std::uniform_int_distribution<uint16_t>  distr(from, to);
  return distr(generator);
}

void MuddleLearnerNetworker::start()
{
  if(nm_mine_) 
    nm_->Start();
  
  upds_out_->Start();
  
  for (auto& upd_in : upds_in_)
  {
    while (!upd_in->is_alive())
    {
      std::cout << "Waiting for client to connect" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
   
   /*
   upd_istream->Subscribe(protocols::FetchProtocols::SUBSCRIBE_PROTO,
                                  protocols::SubscribeProto::NEW_MESSAGE,
                                  new service::Function<void(std::string)>([this](byte_array::ByteArray const &upd) {
                                    auto update = std::make_shared<fetch::dmlf::Update<int>>();
                                    update->deserialise(upd);
                                    std::cout << "Got update: " << update->TimeStamp() << std::endl;
                                    {
                                      Lock lock{updates_m_};
                                      updates_.push(update);
                                    }
                                  }));
  */
  }
}

void MuddleLearnerNetworker::pushUpdate( std::shared_ptr<IUpdate> update)
{
 update->serialise();
 //updates_ostream_->SendUpdate(upd_bytes);
}

std::size_t MuddleLearnerNetworker::getUpdateCount() const 
{
  Lock lock{updates_m_};
  return updates_.size();
}

std::shared_ptr<IUpdate> MuddleLearnerNetworker::getUpdate()
{
  Lock lock{updates_m_};
  if(!updates_.empty())
  {
    std::shared_ptr<IUpdate> upd = updates_.top();
    updates_.pop();
    return upd;
  }
  return std::shared_ptr<IUpdate>{nullptr}; 
}

std::size_t MuddleLearnerNetworker::getCount()
{
  return getUpdateCount();
}

}  // namepsace dmlf
}  // namespace fetch
