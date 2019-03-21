#ifndef __SRC_BLOCK_H__
#define __SRC_BLOCK_H__

#include <stdint.h>

#include <transaction.h>
#include <uint256.h>

class Block {
    public:
        // parameter restrictions

        // constructor and destructor
        Block()=default;
        Block(const Block&) = default;
        ~Block() {};

        // daily utils
        // solve the block maht puzzle
        void Solve();
        // verify block content syntactically
        bool Verify();
        void AddTransaction(const Transaction& t);

        // get & set methods

    private:
        // header
        int32_t nVersion_;
        uint256 hashMilestoneBlock_;
        uint256 hashPrevBlock_;
        uint256 hashTipBlock_;
        uint256 hashTransaction_;
        uint32_t nTime_;
        uint32_t nBits_; // difficultyTarget
        uint32_t nNonce_;

        // content
        std::shared_ptr<Transaction> ptx_;
};

struct BlockIndex {
    // Only one of ptr_block and file_descriptor (together with offset)
    // should be assigned a value
    Block* ptr_block;
    int file_descriptor;
    uint32_t offset; // offset in file
};

#endif
