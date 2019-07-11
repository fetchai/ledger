#pragma once

#include "rbc_envelop.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "ledger/chain/address.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/rpc/server.hpp"

namespace fetch {
namespace dkg {
namespace rbc {

    class RBC {
    public:
        using Endpoint       = muddle::MuddleEndpoint;
        using ConstByteArray = byte_array::ConstByteArray;
        using MuddleAddress  = ConstByteArray;
        using CabinetMembers = std::set<MuddleAddress>;
        using Subscription    = muddle::Subscription;
        using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;

        explicit RBC(Endpoint &endpoint, MuddleAddress address, const CabinetMembers &cabinet, uint32_t threshold);
        void sendRBroadcast(const SerialisedMessage &msg);
        void onRBC(MuddleAddress const &from, RBCEnvelop const &envelop, MuddleAddress const &transmitter);
    private:
        // structs useful in RBC
        struct MsgFlags {
            bool send_, echo_, ready_, request_, answer_;

            MsgFlags() : send_{false}, echo_{false}, ready_{false}, request_{false}, answer_{false} {}
        };

        struct MsgCount {
            size_t e_d_, r_d_;

            MsgCount() : e_d_{0}, r_d_{0} {}
        };

        struct Broadcast { // information per tag
            SerialisedMessage mbar_; //message
            TruncatedHash dbar_; //hash of message
            std::unordered_map<TruncatedHash, MsgCount> msgsCount_;
        };

        struct Party { // information per party
            std::unordered_map<TagType, MsgFlags> flags_;
            uint8_t deliver_s_;
            std::map<uint8_t, RBCMessage&> undelivered_msg; //indexed by tag
            Party() {
                deliver_s_ = 1; // initialize sequence counter by 1
            }
        };

        uint32_t num_parties_{0}; ///< Number of participants in broadcast network
        uint32_t threshold_{0}; ///< Number of byzantine nodes

        uint32_t id_;              ///< Should this match BLS ID (derived from muddle address?)
        uint8_t s{0}; //sequence counter
        std::vector<Party> parties_;
        std::unordered_map<uint64_t, Broadcast> broadcasts_; ///<map from tag to broadcasts

        // For broadcast
        static constexpr uint16_t SERVICE_DKG = 5001;
        static constexpr uint16_t CHANNEL_BROADCAST = 2; ///< Replaces what was previously uint8_t channelId_

        // From before but useful
        MuddleAddress const address_;         ///< Our muddle address
        Endpoint &endpoint_;        ///< The muddle endpoint to communicate on
        CabinetMembers current_cabinet_;     ///< The set of muddle addresses of the cabinet (including our own)
        SubscriptionPtr rbc_subscription_;   ///< For receiving messages in the rbc channel

        void send(const RBCEnvelop &env, const MuddleAddress &address);
        void broadcast(const RBCEnvelop &env);
        void onRBroadcast(std::shared_ptr<RBroadcast> msg_ptr, uint32_t l);
        void onREcho(std::shared_ptr<REcho> msg_ptr, uint32_t l);
        void onRReady(std::shared_ptr<RReady> msg_ptr, uint32_t l);
        void onRRequest(std::shared_ptr<RRequest> msg_ptr, uint32_t l);
        void onRAnswer(std::shared_ptr<RAnswer> msg_ptr, uint32_t l);
        void deliver(const SerialisedMessage &msg, uint32_t l);

        uint32_t cabinetIndex(const MuddleAddress &other_address) const;
        bool checkTag(RBCMessage &msg);
        MsgFlags &partyFlag(uint32_t l, TagType tag);
    };
}  // namespace rbc
}  // namespace dkg
}  // namespace fetch
