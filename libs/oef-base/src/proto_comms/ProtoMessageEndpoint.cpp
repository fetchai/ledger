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

#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/proto_comms/ProtoMessageReader.hpp"
#include "oef-base/proto_comms/ProtoMessageSender.hpp"
#include "oef-base/proto_comms/ProtoPathMessageReader.hpp"
#include "oef-base/proto_comms/ProtoPathMessageSender.hpp"
#include "oef-base/utils/Uri.hpp"

template <typename TXType, typename Reader, typename Sender>
ProtoMessageEndpoint<TXType, Reader, Sender>::ProtoMessageEndpoint(
    std::shared_ptr<EndpointType> endpoint)
  : EndpointPipe<EndpointBase<TXType>>(std::move(endpoint))
{}

template <typename TXType, typename Reader, typename Sender>
void ProtoMessageEndpoint<TXType, Reader, Sender>::setup(
    std::shared_ptr<ProtoMessageEndpoint> &myself)
{
  std::weak_ptr<ProtoMessageEndpoint> myself_wp = myself;

  protoMessageSender = std::make_shared<Sender>(myself_wp);
  endpoint->writer   = protoMessageSender;

  protoMessageReader = std::make_shared<Reader>(myself_wp);
  endpoint->reader   = protoMessageReader;
}

template <typename TXType, typename Reader, typename Sender>
void ProtoMessageEndpoint<TXType, Reader, Sender>::SetEndianness(
    fetch::oef::base::Endianness newstate)
{
  protoMessageReader->SetEndianness(newstate);
  protoMessageSender->SetEndianness(newstate);
}

template class ProtoMessageEndpoint<std::shared_ptr<google::protobuf::Message>>;
template class ProtoMessageEndpoint<std::pair<Uri, std::shared_ptr<google::protobuf::Message>>,
                                    ProtoPathMessageReader, ProtoPathMessageSender>;
