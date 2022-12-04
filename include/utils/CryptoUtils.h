//===- CryptoUtils.h - Cryptographically Secure Pseudo-Random Generator ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains includes and defines for the AES CTR PRNG
// The AES implementation has been derived and adapted
// from libtomcrypt (see http://libtom.org)
// Created on: 22 juin 2012
// Author(s): jrinaldini, pjunod
//===----------------------------------------------------------------------===//

#ifndef LLVM_CryptoUtils_H
#define LLVM_CryptoUtils_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <llvm/Support/ManagedStatic.h>

namespace llvm {

#define CryptoUtils_POOL_SIZE (0x1 << 17) // 2^17

    class CryptoUtils {
    public:
        CryptoUtils();

        ~CryptoUtils();

        char *get_seed();

        void get_bytes(char *buffer, int len);

        char get_char();

        void prng_seed();

        void prng_seed(const std::string &seed);

        // Returns a uniformly distributed 8-bit value
        uint8_t get_uint8_t();

        // Returns a uniformly distributed 32-bit value
        uint32_t get_uint32_t();

        // Returns an integer uniformly distributed on [0, max[
        uint32_t get_range(uint32_t max);

        // Returns a uniformly distributed 64-bit value
        uint64_t get_uint64_t();

        // Scramble a 32-bit value depending on a 128-bit value
        static unsigned scramble32(unsigned in, const char key[16]);

        static int sha256(const char *msg, unsigned char *hash);

    private:
        uint32_t ks[44]{};
        char key[16]{};
        char ctr[16]{};
        char pool[CryptoUtils_POOL_SIZE]{};
        uint32_t idx{};
        std::string seed;
        bool seeded;

        typedef struct {
            uint64_t length;
            uint32_t state[8], curlen;
            unsigned char buf[64];
        } sha256_state;

        static void aes_compute_ks(uint32_t *ks, const char *k);

        static void aes_encrypt(char *out, const char *in, const uint32_t *ks);

        void inc_ctr();

        void populate_pool();

        static int sha256_done(sha256_state *md, unsigned char *out);

        static int sha256_init(sha256_state *md);

        static int sha256_compress(sha256_state *md, const unsigned char *buf);

        static int sha256_process(sha256_state *md, const unsigned char *in,
                                  unsigned long inlen);
    };

    extern ManagedStatic<CryptoUtils> crypto;
} // namespace llvm

#endif // LLVM_CryptoUtils_H
