//===- CryptoUtils.cpp - AES-based Pseudo-Random Generator ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements an AES-CTR-based cryptographically secure
// pseudo-random generator.
//
// Created on: June 22, 2012
// Last modification: November 15, 2013
// Author(s): jrinaldini, pjunod
//
//===----------------------------------------------------------------------===//

#include "utils/CryptoUtils.h"
#include <utils/CryptoUtilsInternal.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>

// Stats
#define DEBUG_TYPE "CryptoUtils"
STATISTIC(statsGetBytes, "a. Number of calls to get_bytes ()");
STATISTIC(statsGetChar, "b. Number of calls to get_char ()");
STATISTIC(statsGetUint8, "c. Number of calls to get_uint8_t ()");
STATISTIC(statsGetUint32, "d. Number of calls to get_uint32_t ()");
STATISTIC(statsGetUint64, "e. Number of calls to get_uint64_t ()");
STATISTIC(statsGetRange, "f. Number of calls to get_range ()");
STATISTIC(statsPopulate, "g. Number of calls to populate ()");
STATISTIC(statsAESEncrypt, "h. Number of calls to aes_encrypt ()");

using namespace llvm;

namespace llvm{
    ManagedStatic<CryptoUtils> crypto;
}

CryptoUtils::CryptoUtils() { seeded = false; }

unsigned CryptoUtils::scramble32(const unsigned in, const char key[16]) {
    assert(key != nullptr && "CryptoUtils::scramble key=nullptr");

    unsigned tmpA, tmpB;

    // Orr, Nathan or Adi can probably break it, but who cares?

    // Round 1
    tmpA = 0x0;
    tmpA ^= AES_PRECOMP_TE0[((in >> 24) ^ key[0]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE1[((in >> 16) ^ key[1]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE2[((in >> 8) ^ key[2]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE3[((in >> 0) ^ key[3]) & 0xFF];

    // Round 2
    tmpB = 0x0;
    tmpB ^= AES_PRECOMP_TE0[((tmpA >> 24) ^ key[4]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE1[((tmpA >> 16) ^ key[5]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE2[((tmpA >> 8) ^ key[6]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE3[((tmpA >> 0) ^ key[7]) & 0xFF];

    // Round 3
    tmpA = 0x0;
    tmpA ^= AES_PRECOMP_TE0[((tmpB >> 24) ^ key[8]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE1[((tmpB >> 16) ^ key[9]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE2[((tmpB >> 8) ^ key[10]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE3[((tmpB >> 0) ^ key[11]) & 0xFF];

    // Round 4
    tmpB = 0x0;
    tmpB ^= AES_PRECOMP_TE0[((tmpA >> 24) ^ key[12]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE1[((tmpA >> 16) ^ key[13]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE2[((tmpA >> 8) ^ key[14]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE3[((tmpA >> 0) ^ key[15]) & 0xFF];

    LOAD32H(tmpA, key)

    return tmpA ^ tmpB;
}

void CryptoUtils::prng_seed(const std::string &_seed) {
    unsigned char s[16];
    unsigned int i = 0;

    /* We accept a prefix "0x" */
    if (!(_seed.size() == 32 || _seed.size() == 34)) {
        errs() << Twine(
                "The AES-CTR PRNG seeding mechanism is expecting a 16-byte value "
                "expressed in hexadecimal, like DEAD....BEEF");
        abort();
    }

    seed = _seed;

    if (_seed.size() == 34) {
        // Assuming that the two first characters are "0x"
        i = 2;
    }

    for (; i < _seed.length(); i += 2) {
        std::string byte = _seed.substr(i, 2);
        s[i >> 1] = (unsigned char) (int) strtol(byte.c_str(), nullptr, 16);
    }

    // _seed is defined to be the
    // key initial value
    memcpy(key, s, 16);
    DEBUG_WITH_TYPE("cryptoutils", dbgs()
            << "CPNRG seeded with " << _seed << "\n");

    // ctr is initialized to all-zeroes
    memset(ctr, 0, 16);

    // Once the seed is there, we compute the
    // AES128 key-schedule
    aes_compute_ks(ks, key);

    seeded = true;

    // We are now ready to fill the pool with
    // cryptographically secure pseudo-random
    // values.
    populate_pool();
}

CryptoUtils::~CryptoUtils() {
    // Some wiping work here
    memset(key, 0, 16);
    memset(ks, 0, 44 * sizeof(uint32_t));
    memset(ctr, 0, 16);
    memset(pool, 0, CryptoUtils_POOL_SIZE);

    idx = 0;
}

void CryptoUtils::populate_pool() {

    statsPopulate++;

    for (int i = 0; i < CryptoUtils_POOL_SIZE; i += 16) {

        // ctr += 1
        inc_ctr();

        // We then encrypt the counter
        aes_encrypt(pool + i, ctr, ks);
    }

    // Reinitializing the index of the first
    // available pseudo-random byte
    idx = 0;
}

#if defined(_WIN64) || defined(_WIN32)
// sic! don't change include order
#include <wincrypt.h>
#include <windows.h>

struct WinDevRandom {
  WinDevRandom() : m_hcryptProv{0}, m_last_read{0} {
    assert(!m_hcryptProv);
    if (!CryptAcquireContext(&m_hcryptProv, nullptr, nullptr, PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT)) {
      errs() << "CryptAcquireContext failed (LastError: " << GetLastError()
             << ")\n";
    } else {
      assert(m_hcryptProv);
    }
  }

  ~WinDevRandom() { close(); }

  std::size_t read(char *key, std::size_t sz) {
    assert(m_hcryptProv);
    if (!CryptGenRandom(m_hcryptProv, sz, reinterpret_cast<BYTE *>(key))) {
      errs() << "CryptGenRandom failed (LastError: " << GetLastError() << ")\n";
    }
    m_last_read = sz;
    return sz;
  }

  void close() {
    if (m_hcryptProv && !CryptReleaseContext(m_hcryptProv, 0)) {
      errs() << "CryptReleaseContext failed (LastError: " << GetLastError()
             << ")\n";
    }
    m_hcryptProv = 0;
    assert(!m_hcryptProv);
  }

  std::size_t gcount() { return m_last_read; }

  explicit operator bool() { return true; }

  bool good() const { return m_hcryptProv; }

private:
  HCRYPTPROV m_hcryptProv;
  std::size_t m_last_read;
};
#endif

void CryptoUtils::prng_seed() {
#if defined(__linux__)
    std::ifstream devrandom("/dev/urandom");
#elif defined(_WIN64) || defined(_WIN32)
    std::string const dev = "CryptGenRandom";
    WinDevRandom devrandom;
#else
    std::ifstream devrandom("/dev/random");
#endif
    if (devrandom) {
        devrandom.read(key, 16);

        if (devrandom.gcount() != 16) {
            errs() << Twine("Cannot read enough bytes in /dev/random");
        }

        devrandom.close();
        DEBUG_WITH_TYPE("cryptoutils",
                        dbgs() << "cryptoutils seeded with /dev/random\n");

        memset(ctr, 0, 16);

        // Once the seed is there, we compute the
        // AES128 key-schedule
        aes_compute_ks(ks, key);

        seeded = true;
    } else {
        errs() << Twine("Cannot open /dev/random");
    }
    populate_pool();
}

void CryptoUtils::inc_ctr() {
    uint64_t iseed;

    LOAD64H(iseed, ctr + 8)
    ++iseed;
    STORE64H(ctr + 8, iseed)
}

char *CryptoUtils::get_seed() {

    if (seeded) {
        return key;
    } else {
        return nullptr;
    }
}

void CryptoUtils::get_bytes(char *buffer, const int len) {

    int sofar = 0, available = 0;

    assert(buffer != nullptr && "CryptoUtils::get_bytes buffer=nullptr");
    assert(len > 0 && "CryptoUtils::get_bytes len <= 0");

    statsGetBytes++;

    if (len > 0) {

        // If the PRNG is not seeded, it the very last time to do it !
        if (!seeded) {
            prng_seed();
        }

        do {
            if (idx + (len - sofar) >= CryptoUtils_POOL_SIZE) {
                // We don't have enough bytes ready in the pool,
                // so let's use the available ones and repopulate !
                available = CryptoUtils_POOL_SIZE - idx;
                memcpy(buffer + sofar, pool + idx, available);
                sofar += available;
                populate_pool();
            } else {
                memcpy(buffer + sofar, pool + idx, len - sofar);
                idx += len - sofar;
                // This will trigger a loop exit
                sofar = len;
            }
        } while (sofar < (len - 1));
    }
}

uint8_t CryptoUtils::get_uint8_t() {
    char ret;

    statsGetUint8++;

    get_bytes(&ret, 1);

    return (uint8_t) ret;
}

char CryptoUtils::get_char() {
    char ret;

    statsGetChar++;

    get_bytes(&ret, 1);

    return ret;
}

uint32_t CryptoUtils::get_uint32_t() {
    char tmp[4];
    uint32_t ret = 0;

    statsGetUint32++;

    get_bytes(tmp, 4);

    LOAD32H(ret, tmp)

    return ret;
}

uint64_t CryptoUtils::get_uint64_t() {
    char tmp[8];
    uint64_t ret = 0;

    statsGetUint64++;

    get_bytes(tmp, 8);

    LOAD64H(ret, tmp)

    return ret;
}

uint32_t CryptoUtils::get_range(const uint32_t max) {
    uint32_t log, r, mask;

    statsGetRange++;

    if (max == 0) {
        return 0;
    } else {
        // Computing the above power of two
        log = 32;
        int i = 0;
        // This loop will terminate, as there is at least one
        // bit set somewhere in max
        while (!(max & masks[i++])) {
            log -= 1;
        }
        mask = (0x1UL << log) - 1;

        // This should loop two times in average
        do {
            r = get_uint32_t() & mask;
        } while (r >= max);

        return r;
    }
}

void CryptoUtils::aes_compute_ks(uint32_t *ks, const char *k) {
    int i;
    uint32_t *p, tmp;

    assert(ks != nullptr);
    assert(k != nullptr);

    LOAD32H(ks[0], k)
    LOAD32H(ks[1], k + 4)
    LOAD32H(ks[2], k + 8)
    LOAD32H(ks[3], k + 12)

    p = ks;
    i = 0;
    while (true) {
        tmp = p[3];
        tmp = ((AES_TE4_3(BYTE(tmp, 2))) ^ (AES_TE4_2(BYTE(tmp, 1))) ^
               (AES_TE4_1(BYTE(tmp, 0))) ^ (AES_TE4_0(BYTE(tmp, 3))));

        p[4] = p[0] ^ tmp ^ AES_RCON[i];
        p[5] = p[1] ^ p[4];
        p[6] = p[2] ^ p[5];
        p[7] = p[3] ^ p[6];
        if (++i == 10) {
            break;
        }
        p += 4;
    }
}

void CryptoUtils::aes_encrypt(char *out, const char *in, const uint32_t *ks) {
    uint32_t state0, state1, state2, state3;
    uint32_t tmp0, tmp1, tmp2, tmp3;
    int i;
    uint32_t r;

    statsAESEncrypt++;

    r = 0;
    LOAD32H(state0, in + 0)
    LOAD32H(state1, in + 4)
    LOAD32H(state2, in + 8)
    LOAD32H(state3, in + 12)

    state0 ^= ks[r + 0];
    state1 ^= ks[r + 1];
    state2 ^= ks[r + 2];
    state3 ^= ks[r + 3];

    i = 0;
    while (true) {
        r += 4;

        tmp0 = AES_TE0(BYTE(state0, 3)) ^ AES_TE1(BYTE(state1, 2)) ^
               AES_TE2(BYTE(state2, 1)) ^ AES_TE3(BYTE(state3, 0)) ^ ks[r + 0];

        tmp1 = AES_TE0(BYTE(state1, 3)) ^ AES_TE1(BYTE(state2, 2)) ^
               AES_TE2(BYTE(state3, 1)) ^ AES_TE3(BYTE(state0, 0)) ^ ks[r + 1];

        tmp2 = AES_TE0(BYTE(state2, 3)) ^ AES_TE1(BYTE(state3, 2)) ^
               AES_TE2(BYTE(state0, 1)) ^ AES_TE3(BYTE(state1, 0)) ^ ks[r + 2];

        tmp3 = AES_TE0(BYTE(state3, 3)) ^ AES_TE1(BYTE(state0, 2)) ^
               AES_TE2(BYTE(state1, 1)) ^ AES_TE3(BYTE(state2, 0)) ^ ks[r + 3];

        if (i == 8) {
            break;
        }
        i++;
        state0 = tmp0;
        state1 = tmp1;
        state2 = tmp2;
        state3 = tmp3;
    }

    r += 4;
    state0 = (AES_TE4_3(BYTE(tmp0, 3))) ^ (AES_TE4_2(BYTE(tmp1, 2))) ^
             (AES_TE4_1(BYTE(tmp2, 1))) ^ (AES_TE4_0(BYTE(tmp3, 0))) ^ ks[r + 0];

    state1 = (AES_TE4_3(BYTE(tmp1, 3))) ^ (AES_TE4_2(BYTE(tmp2, 2))) ^
             (AES_TE4_1(BYTE(tmp3, 1))) ^ (AES_TE4_0(BYTE(tmp0, 0))) ^ ks[r + 1];

    state2 = (AES_TE4_3(BYTE(tmp2, 3))) ^ (AES_TE4_2(BYTE(tmp3, 2))) ^
             (AES_TE4_1(BYTE(tmp0, 1))) ^ (AES_TE4_0(BYTE(tmp1, 0))) ^ ks[r + 2];

    state3 = (AES_TE4_3(BYTE(tmp3, 3))) ^ (AES_TE4_2(BYTE(tmp0, 2))) ^
             (AES_TE4_1(BYTE(tmp1, 1))) ^ (AES_TE4_0(BYTE(tmp2, 0))) ^ ks[r + 3];

    STORE32H(out + 0, state0)
    STORE32H(out + 4, state1)
    STORE32H(out + 8, state2)
    STORE32H(out + 12, state3)
}

int CryptoUtils::sha256_process(sha256_state *md, const unsigned char *in,
                                unsigned long inlen) {
    unsigned long n;
    int err;
    assert(md != nullptr && "CryptoUtils::sha256_process md=nullptr");
    assert(in != nullptr && "CryptoUtils::sha256_process in=nullptr");

    if (md->curlen > sizeof(md->buf)) {
        return 1;
    }
    while (inlen > 0) {
        if (md->curlen == 0 && inlen >= 64) {
            if ((err = sha256_compress(md, in)) != 0) {
                return err;
            }
            md->length += 64 * 8;
            in += 64;
            inlen -= 64;
        } else {
            n = MIN(inlen, (64 - md->curlen));
            memcpy(md->buf + md->curlen, in, (size_t) n);
            md->curlen += n;
            in += n;
            inlen -= n;
            if (md->curlen == 64) {
                if ((err = sha256_compress(md, md->buf)) != 0) {
                    return err;
                }
                md->length += 8 * 64;
                md->curlen = 0;
            }
        }
    }
    return 0;
}

int CryptoUtils::sha256_compress(sha256_state *md, const unsigned char *buf) {
    uint32_t S[8], W[64], t0, t1;
    int i;

    /* copy state into S */
    for (i = 0; i < 8; i++) {
        S[i] = md->state[i];
    }

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4 * i))
    }

    /* fill W[16..63] */
    for (i = 16; i < 64; i++) {
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }

    /* Compress */

    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 0, 0x428a2f98)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 1, 0x71374491)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 2, 0xb5c0fbcf)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 3, 0xe9b5dba5)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 4, 0x3956c25b)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 5, 0x59f111f1)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 6, 0x923f82a4)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 7, 0xab1c5ed5)
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 8, 0xd807aa98)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 9, 0x12835b01)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 10, 0x243185be)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 11, 0x550c7dc3)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 12, 0x72be5d74)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 13, 0x80deb1fe)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 14, 0x9bdc06a7)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 15, 0xc19bf174)
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 16, 0xe49b69c1)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 17, 0xefbe4786)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 18, 0x0fc19dc6)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 19, 0x240ca1cc)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 20, 0x2de92c6f)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 21, 0x4a7484aa)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 22, 0x5cb0a9dc)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 23, 0x76f988da)
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 24, 0x983e5152)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 25, 0xa831c66d)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 26, 0xb00327c8)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 27, 0xbf597fc7)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 28, 0xc6e00bf3)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 29, 0xd5a79147)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 30, 0x06ca6351)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 31, 0x14292967)
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 32, 0x27b70a85)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 33, 0x2e1b2138)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 34, 0x4d2c6dfc)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 35, 0x53380d13)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 36, 0x650a7354)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 37, 0x766a0abb)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 38, 0x81c2c92e)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 39, 0x92722c85)
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 40, 0xa2bfe8a1)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 41, 0xa81a664b)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 42, 0xc24b8b70)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 43, 0xc76c51a3)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 44, 0xd192e819)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 45, 0xd6990624)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 46, 0xf40e3585)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 47, 0x106aa070)
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 48, 0x19a4c116)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 49, 0x1e376c08)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 50, 0x2748774c)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 51, 0x34b0bcb5)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 52, 0x391c0cb3)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 53, 0x4ed8aa4a)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 54, 0x5b9cca4f)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 55, 0x682e6ff3)
    RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 56, 0x748f82ee)
    RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 57, 0x78a5636f)
    RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 58, 0x84c87814)
    RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 59, 0x8cc70208)
    RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 60, 0x90befffa)
    RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 61, 0xa4506ceb)
    RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 62, 0xbef9a3f7)
    RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 63, 0xc67178f2)

    /* feedback */
    for (i = 0; i < 8; i++) {
        md->state[i] = md->state[i] + S[i];
    }
    return 0;
}

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int CryptoUtils::sha256_init(sha256_state *md) {
    assert(md != nullptr && "CryptoUtils::sha256_init md=nullptr");

    md->curlen = 0;
    md->length = 0;
    md->state[0] = 0x6A09E667UL;
    md->state[1] = 0xBB67AE85UL;
    md->state[2] = 0x3C6EF372UL;
    md->state[3] = 0xA54FF53AUL;
    md->state[4] = 0x510E527FUL;
    md->state[5] = 0x9B05688CUL;
    md->state[6] = 0x1F83D9ABUL;
    md->state[7] = 0x5BE0CD19UL;
    return 0;
}

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (32 bytes)
   @return CRYPT_OK if successful
*/
int CryptoUtils::sha256_done(sha256_state *md, unsigned char *out) {
    int i;

    assert(md != nullptr && "CryptoUtils::sha256_done md=nullptr");
    assert(out != nullptr && "CryptoUtils::sha256_done out=nullptr");

    if (md->curlen >= sizeof(md->buf)) {
        return 1;
    }

    /* increase the length of the message */
    md->length += md->curlen * 8;

    /* append the '1' bit */
    md->buf[md->curlen++] = (unsigned char) 0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->curlen > 56) {
        while (md->curlen < 64) {
            md->buf[md->curlen++] = (unsigned char) 0;
        }
        sha256_compress(md, md->buf);
        md->curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->curlen < 56) {
        md->buf[md->curlen++] = (unsigned char) 0;
    }

    /* store length */
    STORE64H(md->buf + 56, md->length)
    sha256_compress(md, md->buf);

    /* copy output */
    for (i = 0; i < 8; i++) {
        STORE32H(out + (4 * i), md->state[i])
    }
    return 0;
}

int CryptoUtils::sha256(const char *msg, unsigned char *hash) {
    unsigned char tmp[32];
    sha256_state md;

    sha256_init(&md);
    sha256_process(&md, (const unsigned char *) msg,
                   (unsigned long) strlen((const char *) msg));
    sha256_done(&md, tmp);

    memcpy(hash, tmp, 32);
    return 0;
}
