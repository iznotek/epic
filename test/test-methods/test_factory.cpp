// Copyright (c) 2019 EPI-ONE Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test_factory.h"
#include "miner.h"
#include "random.h"
#include "test_env.h"

#include <cstring>
#include <memory>

std::string TestFactory::GetRandomString(size_t len) {
    static const char alph[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";
    char s[len + 1];
    for (size_t i = 0; i < len; ++i) {
        s[i] = alph[GetRand() % (sizeof(alph) - 1)];
    }
    s[len] = 0;
    return std::string(s);
}

uint256 TestFactory::CreateRandomHash() {
    /* draw 64 bit */
    unsigned long long x;
    GetOpenSSLRand((unsigned char*) &x, sizeof(x));

    std::vector<unsigned char> vch(Hash::SIZE);
    for (std::size_t i = 0; i < 8; i++) {
        vch[i] = ((uint8_t*) &x)[i];
    }

    return uint256(vch);
}

std::pair<CKey, CPubKey> TestFactory::CreateKeyPair(bool compressed) {
    CKey seckey    = CKey().MakeNewKey(compressed);
    CPubKey pubkey = seckey.GetPubKey();
    return std::make_pair(std::move(seckey), std::move(pubkey));
}

std::pair<uint256, std::vector<unsigned char>> TestFactory::CreateSig(const CKey& privateKey) {
    auto msg     = GetRandomString(10);
    auto hashMsg = HashSHA2<1>(msg.data(), msg.size());
    std::vector<unsigned char> sig;
    privateKey.Sign(hashMsg, sig);
    return std::make_pair(hashMsg, sig);
}

Transaction TestFactory::CreateTx(int numTxInput, int numTxOutput) {
    Transaction tx;
    uint32_t maxPos = GetRand() % 128 + 1;
    for (int i = 0; i < numTxInput; ++i) {
        tx.AddInput(TxInput(CreateRandomHash(), i % maxPos, i % maxPos, Listing(std::vector<unsigned char>(i))));
    }

    for (int i = 0; i < numTxOutput; ++i) {
        tx.AddOutput(TxOutput(i + 1, Listing(std::vector<unsigned char>(i))));
    }
    tx.FinalizeHash();
    return tx;
}

Block TestFactory::CreateBlock(int numTxInput, int numTxOutput, bool finalize, int maxTxns) {
    Block b = Block(GetParams().version, CreateRandomHash(), CreateRandomHash(), CreateRandomHash(), uint256(),
                    timeGenerator.NextTime(), GENESIS_VERTEX->snapshot->blockTarget.GetCompact(), 0);

    if (numTxInput && numTxOutput) {
        for (int i = 0; i < maxTxns; ++i) {
            b.AddTransaction(CreateTx(numTxInput, numTxOutput));
        }
    }

    b.SetMerkle();
    b.CalculateOptimalEncodingSize();

    if (finalize) {
        b.FinalizeHash();
        return b;
    } else {
        TestBlock tb(std::move(b), this);
        return static_cast<Block&>(tb);
    }
}

ConstBlockPtr TestFactory::CreateBlockPtr(int numTxInput, int numTxOutput, bool finalize, int maxTxns) {
    return std::make_shared<const Block>(CreateBlock(numTxInput, numTxOutput, finalize, maxTxns));
}

Vertex TestFactory::CreateVertex(ConstBlockPtr b) {
    Vertex vtx(b);
    // Set extra info
    vtx.minerChainHeight = GetRand();
    vtx.cumulativeReward = Coin(GetRand());

    if (GetRand() % 2) {
        vtx.validity.push_back(Vertex::VALID);
    } else {
        vtx.validity.push_back(Vertex::INVALID);
    }

    return vtx;
}

VertexPtr TestFactory::CreateVertexPtr(int numTxInput, int numTxOutput, bool finalize, int maxTxns) {
    return std::make_shared<Vertex>(CreateVertex(CreateBlockPtr(numTxInput, numTxOutput, finalize, maxTxns)));
}

VertexPtr TestFactory::CreateConsecutiveVertexPtr(uint32_t timeToset, Miner& m) {
    Block b = CreateBlock(0, 0, false);
    b.SetTime(timeToset);

    if (!m.IsRunning()) {
        m.Start();
    }

    do {
        b.SetNonce(b.GetNonce() + 1);
        m.Solve(b);
    } while (UintToArith256(b.GetHash()) > GENESIS_VERTEX->snapshot->milestoneTarget);

    return std::make_shared<Vertex>(std::move(b));
}

MilestonePtr TestFactory::CreateMilestonePtr(MilestonePtr previous, Vertex& vertex, std::vector<VertexWPtr>&& lvs) {
    return CreateNextMilestone(previous, vertex, std::move(lvs));
}

MilestonePtr TestFactory::CreateMilestonePtr(MilestonePtr previous, VertexPtr& pVtx) {
    return CreateNextMilestone(previous, *pVtx, std::vector<VertexWPtr>{pVtx});
}

TestChain TestFactory::CreateChain(const VertexPtr& startMs, size_t height, bool tx) {
    assert(height);
    VertexPtr lastMs    = startMs;
    VertexPtr prevBlock = startMs;

    TestChain testChain{{}};

    SetLogLevel(SPDLOG_LEVEL_OFF);
    Miner m(1);
    m.Start();

    size_t count = 0;
    TimeGenerator timeg{startMs->cblock->GetTime(), 1, GetRand() % 10 + 2, GetRand()};
    while (count < height) {
        Block b{GetParams().version};
        b.SetMilestoneHash(lastMs->cblock->GetHash());
        b.SetPrevHash(prevBlock->cblock->GetHash());
        if (testChain.size() == 1) {
            b.SetTipHash(GENESIS->GetHash());
        } else {
            b.SetTipHash(testChain[GetRand() % (testChain.size() - 1)][0]->cblock->GetHash());
        }
        b.SetTime(timeg.NextTime());
        b.SetDifficultyTarget(lastMs->snapshot->blockTarget.GetCompact());

        // Special transaction on the first registration block
        if (b.GetPrevHash() == GENESIS->GetHash()) {
            Transaction tx = Transaction{CreateKeyPair().second.GetID()};
            b.AddTransaction(tx);
        } else if (tx) {
            for (int i = 0; i < GetRand() % 10; ++i) {
                b.AddTransaction(CreateTx(GetRand() % 10 + 1, GetRand() % 10 + 1));
            }
        }
        b.SetMerkle();
        b.CalculateOptimalEncodingSize();
        m.Solve(b);

        ConstBlockPtr blkptr = std::make_shared<const Block>(std::move(b));
        VertexPtr node       = std::make_shared<Vertex>(blkptr);

        // Set proper info in node
        node->height           = lastMs->height + 1;
        node->minerChainHeight = prevBlock->minerChainHeight + 1;
        node->validity.resize(blkptr->GetTransactionSize());
        memset(node->validity.data(), Vertex::Validity::VALID, blkptr->GetTransactionSize());

        prevBlock = node;
        testChain.back().emplace_back(node);

        if (CheckMsPOW(blkptr, lastMs->snapshot)) {
            // Prepare the lvs of the milestone
            std::vector<VertexWPtr> lvs;
            const auto& lvs_vtcs = testChain.back();
            lvs.reserve(lvs_vtcs.size());
            std::transform(lvs_vtcs.begin(), lvs_vtcs.end(), std::back_inserter(lvs),
                           [](VertexPtr p) -> VertexWPtr { return VertexWPtr(p); });

            CreateNextMilestone(lastMs->snapshot, *node, std::move(lvs));
            lastMs = std::move(node);

            if (count++ < height - 1) {
                testChain.emplace_back();
            }
        }
    }

    m.Stop();
    ResetLogLevel();
    // PrintChain(testChain);
    return testChain;
}

TestChain TestFactory::CreateChain(const Vertex& startMs, size_t height, bool tx) {
    return CreateChain(std::make_shared<Vertex>(startMs), height, tx);
}

std::tuple<TestRawChain, std::vector<VertexPtr>> TestFactory::CreateRawChain(const VertexPtr& startMs,
                                                                             size_t height,
                                                                             bool tx) {
    auto chain = CreateChain(startMs, height, tx);

    TestRawChain rawChain;
    rawChain.reserve(chain.size());
    std::vector<VertexPtr> milestons;
    milestons.reserve(chain.size());

    std::transform(chain.begin(), chain.end(), std::back_inserter(rawChain), [&milestons](LevelSetVtxs lvs) {
        // extract milestone
        milestons.emplace_back(lvs.back());

        // convert each block from VertexPtr to ConstBlockPtr
        LevelSetBlks rawLvs;
        rawLvs.reserve(lvs.size());
        std::transform(lvs.begin(), lvs.end(), std::back_inserter(rawLvs), [](VertexPtr vtx) { return vtx->cblock; });
        return rawLvs;
    });

    return {rawChain, milestons};
}

std::tuple<TestRawChain, std::vector<VertexPtr>> TestFactory::CreateRawChain(const Vertex& startMs,
                                                                             size_t height,
                                                                             bool tx) {
    return CreateRawChain(std::make_shared<Vertex>(startMs), height, tx);
}
