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

#include "gtest/gtest.h"
#include "shared_functions.hpp"

TEST(MessengerMailboxTest, BasicRegisteringUnregistering)
{
  auto server = NewServerWithFakeMailbox(1337, 1338);

  // Registering a mailbox for every other messenger
  std::vector<std::shared_ptr<Messenger>> messengers;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (uint64_t i = 0; i < 10; ++i)
  {
    auto messenger = NewMessenger(1337);
    messenger->messenger->Register((i & 1) == 0);
    messengers.push_back(messenger);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_EQ(server->mailbox.unregistered_messengers, 0);
  EXPECT_EQ(server->mailbox.registered_messengers, 5);

  // Unregistering
  for (auto messenger : messengers)
  {
    messenger->messenger->Unregister();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  EXPECT_EQ(server->mailbox.unregistered_messengers, 10);
  EXPECT_EQ(server->mailbox.registered_messengers, 5);

  // Teardown
  while (!messengers.empty())
  {
    auto messenger = messengers.back();
    messengers.pop_back();
    messenger->messenger_muddle->Stop();
    messenger->network_manager.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

TEST(MessengerMailboxTest, BilateralCommsMailbox)
{
  auto server = NewServer(1337, 1338);

  // Testing mailbox.
  auto messenger1 = NewMessenger(1337);
  auto messenger2 = NewMessenger(1337);

  messenger1->messenger->Register(true);
  messenger2->messenger->Register(true);

  std::deque<Message> sent_messages1;
  std::deque<Message> sent_messages2;

  for (uint64_t i = 0; i < 10; ++i)
  {
    if (i & 1)
    {
      Message msg;
      msg.from.node      = server->mail_muddle->GetAddress();
      msg.from.messenger = messenger1->messenger_muddle->GetAddress();
      msg.to.node        = server->mail_muddle->GetAddress();
      msg.to.messenger   = messenger2->messenger_muddle->GetAddress();

      messenger1->messenger->SendMessage(msg);
      sent_messages2.push_back(msg);
    }
    else
    {
      Message msg;
      msg.from.node      = server->mail_muddle->GetAddress();
      msg.from.messenger = messenger2->messenger_muddle->GetAddress();
      msg.to.node        = server->mail_muddle->GetAddress();
      msg.to.messenger   = messenger1->messenger_muddle->GetAddress();

      messenger2->messenger->SendMessage(msg);
      sent_messages1.push_back(msg);
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  auto messages1          = server->mailbox.GetMessages(messenger1->messenger_muddle->GetAddress());
  auto messages2          = server->mailbox.GetMessages(messenger2->messenger_muddle->GetAddress());
  auto recevied_messages1 = messenger1->messenger->GetMessages(200);
  auto recevied_messages2 = messenger2->messenger->GetMessages(200);

  EXPECT_EQ(ToSet(messages1), ToSet(recevied_messages1));
  EXPECT_EQ(ToSet(messages1), ToSet(sent_messages1));

  EXPECT_EQ(ToSet(messages2), ToSet(recevied_messages2));
  EXPECT_EQ(ToSet(messages2), ToSet(sent_messages2));
}

/*
  TODO: Things to test
  1) Unregister on timeout (does not exist yet)
  2) Chain based message delivery
  3) HTTP interface
*/

/*
TEST(MessengerMailboxTest, MessagesRouting)
{
  //  std::vector< Server > server_chain;

  //  auto server = NewServer(1337, 1338);
  EXPECT_TRUE(false);
}
*/

/*
TEST(MessengerMailboxTest, DirectDelivery)
{
  EXPECT_TRUE(false);
}

TEST(MessengerMailboxTest, MailboxDelivery)
{
  EXPECT_TRUE(false);
}
*/
