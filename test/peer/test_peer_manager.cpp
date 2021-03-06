// Copyright (c) 2019 EPI-ONE Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include "peer_manager.h"
#include "test_env.h"

class TestPeerManager : public testing::Test {
public:
    PeerManager server;
    PeerManager client;

    static void SetUpTestCase() {
        CONFIG = std::make_unique<Config>();
        EpicTestEnvironment::SetUpDAG("test_peer_manager/");
    }
    static void TearDownTestCase() {
        CONFIG.reset();
        EpicTestEnvironment::TearDownDAG("test_peer_manager/");
    }
    void SetUp() {
        CONFIG->SetAmISeed(true);
        server.Start();
        client.Start();
    }

    void TearDown() {
        CONFIG->SetAmISeed(false);
        server.Stop();
        client.Stop();
    }
};

TEST_F(TestPeerManager, CallBack) {
    ASSERT_TRUE(server.Bind("127.0.0.1"));
    ASSERT_TRUE(server.Listen(43250));
    ASSERT_TRUE(client.ConnectTo("127.0.0.1:43250"));
    usleep(50000);
    EXPECT_EQ(server.GetFullyConnectedPeerSize(), 1);
    EXPECT_EQ(client.GetFullyConnectedPeerSize(), 1);
}

TEST_F(TestPeerManager, CheckHaveConnectedSameIP) {
    ASSERT_TRUE(server.Bind("127.0.0.1"));
    ASSERT_TRUE(server.Listen(43260));
    ASSERT_TRUE(client.ConnectTo("127.0.0.1:43260"));
    usleep(50000);

    PeerManager same_ip_client;
    same_ip_client.Start();
    same_ip_client.ConnectTo("127.0.0.1:43260");
    usleep(50000);
    EXPECT_EQ(server.GetFullyConnectedPeerSize(), 2);
    EXPECT_EQ(same_ip_client.GetConnectedPeerSize(), 1);

    ASSERT_EQ(server.RandomlySelect(2).size(), 2);

    same_ip_client.Stop();
}

TEST_F(TestPeerManager, RelayProtocol) {
    ASSERT_TRUE(server.Bind("127.0.0.1"));
    ASSERT_TRUE(server.Listen(43280));
    usleep(50000);

    TestFactory fac = EpicTestEnvironment::GetFactory();
    auto block      = fac.CreateBlockPtr();
    block->SetCount(1);

    server.RelayBlock(block, nullptr);
    ASSERT_EQ(block->GetCount(), 1);

    ASSERT_TRUE(client.ConnectTo("127.0.0.1:43280"));
    usleep(50000);

    server.RelayBlock(block, nullptr);
    ASSERT_EQ(block->GetCount(), 0);

    server.RelayBlock(block, nullptr);
    ASSERT_EQ(block->GetCount(), 0);
}
