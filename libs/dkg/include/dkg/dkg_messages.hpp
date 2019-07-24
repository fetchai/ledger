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

#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "network/muddle/rpc/client.hpp"

namespace fetch {
namespace dkg {

using DKGSerializer = fetch::serializers::ByteArrayBuffer;

/**
 * Different messages using in distributed key generation (DKG) protocol.
 * Coefficients - contain the broadcast coefficients as strings
 * Shares - contain the secret shares which have been exposed in broadcasts as strings
 * Complaints - contain the set of miners who are being complained against
 */

class DKGMessage
{
public:
  using MsgSignature  = byte_array::ConstByteArray;
  using MuddleAddress = byte_array::ConstByteArray;
  using Coefficient   = std::string;
  using Share         = std::string;
  using CabinetId     = MuddleAddress;

  enum class MessageType : uint8_t
  {
    COEFFICIENT,
    SHARE,
    COMPLAINT
  };

  virtual ~DKGMessage() = default;

  /// @name Getter functions
  /// @{
  MessageType type() const
  {
    return type_;
  }
  MsgSignature signature() const
  {
    return signature_;
  }
  /// @}

  virtual DKGSerializer Serialize() const = 0;

protected:
  const MessageType type_;       ///< Type of message of the three listed above
  MsgSignature      signature_;  ///< ECDSA signature of message

  explicit DKGMessage(MessageType type)
    : type_{type}
  {}
  DKGMessage(MessageType type, MsgSignature sig)
    : type_{type}
    , signature_{std::move(sig)}
  {}
};

class CoefficientsMessage : public DKGMessage
{
  uint8_t                  phase_;         ///< Phase of state machine that this message is for
  std::vector<Coefficient> coefficients_;  ///< Coefficients as strings

public:
  explicit CoefficientsMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::COEFFICIENT}
  {
    serialiser >> phase_ >> coefficients_ >> signature_;
  }
  CoefficientsMessage(uint8_t phase, std::vector<Coefficient> coeff, MsgSignature sig)
    : DKGMessage{MessageType::COEFFICIENT, std::move(sig)}
    , phase_{phase}
    , coefficients_{std::move(coeff)}
  {}
  ~CoefficientsMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << phase_ << coefficients_ << signature_;
    return serializer;
  }

  /// @name Getter functions
  /// @{
  uint8_t phase() const
  {
    return phase_;
  }
  std::vector<Coefficient> const &coefficients() const
  {
    return coefficients_;
  }
  ///@}
};

class SharesMessage : public DKGMessage
{
  uint8_t phase_;  ///< Phase of state machine that this message is for
  std::unordered_map<CabinetId, std::pair<Share, Share>>
      shares_;  ///< Exposed secret shares for a particular committee member
public:
  explicit SharesMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::SHARE}
  {
    serialiser >> phase_ >> shares_ >> signature_;
  }
  SharesMessage(uint8_t phase, std::unordered_map<CabinetId, std::pair<Share, Share>> shares,
                MsgSignature sig)
    : DKGMessage{MessageType::SHARE, std::move(sig)}
    , phase_{phase}
    , shares_{std::move(shares)}
  {}
  ~SharesMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << phase_ << shares_ << signature_;
    return serializer;
  }

  /// @name Getter functions
  /// @{
  uint8_t phase() const
  {
    return phase_;
  }
  std::unordered_map<CabinetId, std::pair<Share, Share>> const &shares() const
  {
    return shares_;
  }
  ///@}
};

class ComplaintsMessage : public DKGMessage
{
  std::unordered_set<CabinetId>
      complaints_;  ///< Committee members that you are complaining against
public:
  // Construction/Destruction
  explicit ComplaintsMessage(DKGSerializer &serialiser)
    : DKGMessage{MessageType::COMPLAINT}
  {
    serialiser >> complaints_ >> signature_;
  }
  ComplaintsMessage(std::unordered_set<CabinetId> complaints, MsgSignature sig)
    : DKGMessage{MessageType::COMPLAINT, std::move(sig)}
    , complaints_{std::move(complaints)}
  {}
  ~ComplaintsMessage() override = default;

  DKGSerializer Serialize() const override
  {
    DKGSerializer serializer;
    serializer << complaints_ << signature_;
    return serializer;
  }

  /// @name Getter functions
  /// @{
  std::unordered_set<CabinetId> const &complaints() const
  {
    return complaints_;
  }
  ///@}
};

class DKGEnvelop
{
  using MessageType = DKGMessage::MessageType;
  using Payload     = byte_array::ConstByteArray;

public:
  DKGEnvelop() = default;
  explicit DKGEnvelop(DKGMessage const &msg)
    : type_{msg.type()}
    , serialisedMessage_{msg.Serialize().data()}
  {}

  template <typename T>
  void Serialize(T &serialiser) const
  {
    serialiser << static_cast<uint8_t>(type_) << serialisedMessage_;
  }

  template <typename T>
  void Deserialize(T &serialiser)
  {
    uint8_t val;
    serialiser >> val;
    type_ = static_cast<MessageType>(val);
    serialiser >> serialisedMessage_;
  }

  std::shared_ptr<DKGMessage> Message() const;

private:
  MessageType type_;               ///< Type of message contained in the envelope
  Payload     serialisedMessage_;  ///< Serialised message
};

template <typename T>
inline void Serialize(T &serializer, DKGEnvelop const &env)
{
  env.Serialize(serializer);
}

template <typename T>
inline void Deserialize(T &serializer, DKGEnvelop &env)
{
  env.Deserialize(serializer);
}

}  // namespace dkg
}  // namespace fetch
