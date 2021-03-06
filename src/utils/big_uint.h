// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2019 EPI-ONE Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include "common.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

/** Template base class for fixed-sized opaque blobs. */
template <unsigned int BITS>
class base_blob {
protected:
    static constexpr int WIDTH = BITS / 8;
    uint8_t data[WIDTH];

public:
    base_blob() noexcept {
        memset(data, 0, sizeof(data));
    }

    explicit base_blob(const std::vector<unsigned char>& vch);
    base_blob(unsigned char* vch, size_t length);

    // Defined to be used with std::atomic
    // note that move operations are still copying
    base_blob(const base_blob<BITS>&)     = default;
    base_blob(base_blob<BITS>&&) noexcept = default;
    base_blob<BITS>& operator=(const base_blob<BITS>&) = default;
    base_blob<BITS>& operator=(base_blob<BITS>&&) noexcept = default;
    ~base_blob()                                           = default;

    bool IsNull() const {
        for (int i = 0; i < WIDTH; i++) {
            if (data[i] != 0) {
                return false;
            }
        }
        return true;
    }

    void SetNull() {
        memset(data, 0, sizeof(data));
    }

    inline int Compare(const base_blob& other) const {
        return memcmp(data, other.data, sizeof(data));
    }

    friend inline bool operator==(const base_blob& a, const base_blob& b) {
        return a.Compare(b) == 0;
    }
    friend inline bool operator!=(const base_blob& a, const base_blob& b) {
        return a.Compare(b) != 0;
    }
    friend inline bool operator<(const base_blob& a, const base_blob& b) {
        return a.Compare(b) < 0;
    }

    std::string GetHex() const;
    void SetHex(const char* psz);
    void SetHex(const std::string& str);

    unsigned char* begin() {
        return &data[0];
    }

    unsigned char* end() {
        return &data[WIDTH];
    }

    const unsigned char* begin() const {
        return &data[0];
    }

    const unsigned char* end() const {
        return &data[WIDTH];
    }

    unsigned int size() const {
        return sizeof(data);
    }

    uint64_t GetUint64(int pos) const {
        const uint8_t* ptr = data + pos * 8;
        return ((uint64_t) ptr[0]) | ((uint64_t) ptr[1]) << 8 | ((uint64_t) ptr[2]) << 16 | ((uint64_t) ptr[3]) << 24 |
               ((uint64_t) ptr[4]) << 32 | ((uint64_t) ptr[5]) << 40 | ((uint64_t) ptr[6]) << 48 |
               ((uint64_t) ptr[7]) << 56;
    }

    template <typename Stream>
    void Serialize(Stream& s) const {
        s.write((char*) data, sizeof(data));
    }

    template <typename Stream>
    void Deserialize(Stream& s) {
        s.read((char*) data, sizeof(data));
    }
};

namespace std {
template <unsigned int BITS>
string to_string(const base_blob<BITS>& uint);
} // namespace std

/** 160-bit opaque blob.
 * @note This type is called uint160 for historical reasons only. It is an
 * opaque blob of 160 bits and has no integer operations.
 */
class uint160 : public base_blob<160> {
public:
    using base_blob::base_blob;
};

/** 256-bit opaque blob.
 * @note This type is called uint256 for historical reasons only. It is an
 * opaque blob of 256 bits and has no integer operations. Use arith_uint256 if
 * those are required.
 */
class uint256 : public base_blob<256> {
public:
    using base_blob::base_blob;

    uint256(const base_blob<256>& b) {
        memcpy(data, b.begin(), WIDTH);
    }

    /** A cheap hash function that just returns 64 bits from the result, it can
     * be used when the contents are considered uniformly random. It is not
     * appropriate when the value can easily be influenced from outside as e.g.
     * a network adversary could provide values to trigger worst-case behavior.
     */
    uint64_t GetCheapHash() const {
        return ReadLE64(data);
    }

    uint160 GetUint160() const {
        std::vector<unsigned char> vch(data, data + 20);
        return uint160(vch);
    }

    std::string to_substr() const {
        return GetHex().substr(0, 8);
    }
};

/* uint256 from const char *.
 * This is a separate function because the constructor uint256(const char*) can
 * result in dangerously catching uint256(0).
 */
template <unsigned int BITS>
inline base_blob<BITS> uintS(const char* str) {
    base_blob<BITS> rv;
    rv.SetHex(str);
    return rv;
}

/* uint256 from std::string.
 * This is a separate function because the constructor uint256(const std::string
 * &str) can result in dangerously catching uint256(0) via std::string(const
 * char*).
 */
template <unsigned int BITS>
inline base_blob<BITS> uintS(const std::string& str) {
    base_blob<BITS> rv;
    rv.SetHex(str);
    return rv;
}

/** Key hasher for unordered_map */
template <>
struct std::hash<uint256> {
    size_t operator()(const uint256& x) const {
        return x.GetCheapHash();
    }
};

class uint512 : public base_blob<512> {
public:
    using base_blob::base_blob;

    uint512(const base_blob<512>& b) {
        memcpy(data, b.begin(), WIDTH);
    }

    std::string to_substr() const {
        return GetHex().substr(0, 16);
    }
};

#endif // BITCOIN_UINT256_H
