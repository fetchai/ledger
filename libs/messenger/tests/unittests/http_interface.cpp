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

#include "gtest/gtest.h"
#include "shared_functions.hpp"

using Address        = fetch::muddle::Packet::Address;
using Message        = fetch::messenger::Message;
using MessageList    = fetch::messenger::Mailbox::MessageList;
using ConstByteArray = fetch::byte_array::ConstByteArray;

TEST(MessengerMailboxTest, BasicHTTPRegisteringUnregistering)
{
  auto server = NewServerWithFakeMailbox(0);

  // Registering a mailbox for every other messenger
  std::vector<std::shared_ptr<HTTPMessenger>> messengers;
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  for (uint64_t i = 0; i < 10; ++i)
  {
    auto messenger = NewHTTPMessenger(8000);
    EXPECT_TRUE(messenger->Register());
    messengers.push_back(messenger);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  EXPECT_EQ(server->mailbox.unregistered_messengers, 0);
  EXPECT_EQ(server->mailbox.registered_messengers, 10);

  // Unregistering
  for (auto const &messenger : messengers)
  {
    messenger->Unregister();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  EXPECT_EQ(server->mailbox.unregistered_messengers, 10);
  EXPECT_EQ(server->mailbox.registered_messengers, 10);

  // Teardown
  while (!messengers.empty())
  {
    auto messenger = messengers.back();
    messengers.pop_back();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  server->http.Stop();
  server->messenger_muddle->Stop();
  server->network_manager.Stop();
}

TEST(MessengerMailboxTest, BilateralHTTPCommsMailbox)
{
  auto server = NewServer(0);
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  // Testing mailbox.
  auto messenger1 = NewHTTPMessenger(8000);
  auto messenger2 = NewHTTPMessenger(8000);

  messenger1->Register();
  messenger2->Register();

  std::deque<Message> sent_messages1;
  std::deque<Message> sent_messages2;

  for (uint64_t i = 0; i < 10; ++i)
  {
    if ((i & 1) != 0)
    {
      Message msg;
      msg.from.node      = server->mail_muddle->GetAddress();
      msg.from.messenger = messenger1->GetAddress();
      msg.to.node        = server->mail_muddle->GetAddress();
      msg.to.messenger   = messenger2->GetAddress();

      messenger1->SendMessage(msg);
      sent_messages2.push_back(msg);
    }
    else
    {
      Message msg;
      msg.from.node      = server->mail_muddle->GetAddress();
      msg.from.messenger = messenger2->GetAddress();
      msg.to.node        = server->mail_muddle->GetAddress();
      msg.to.messenger   = messenger1->GetAddress();

      messenger2->SendMessage(msg);
      sent_messages1.push_back(msg);
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  auto messages1          = server->mailbox.GetMessages(messenger1->GetAddress());
  auto messages2          = server->mailbox.GetMessages(messenger2->GetAddress());
  auto recevied_messages1 = messenger1->GetMessages();
  auto recevied_messages2 = messenger2->GetMessages();

  EXPECT_EQ(ToSet(messages1), ToSet(recevied_messages1));
  EXPECT_EQ(ToSet(messages1), ToSet(sent_messages1));

  EXPECT_EQ(ToSet(messages2), ToSet(recevied_messages2));
  EXPECT_EQ(ToSet(messages2), ToSet(sent_messages2));

  // Shutting down
  server->http.Stop();
  server->messenger_muddle->Stop();
  server->mail_muddle->Stop();
  server->network_manager.Stop();
}
