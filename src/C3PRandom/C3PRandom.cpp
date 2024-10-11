/*
File:   C3PRandom.cpp
Author: J. Ian Lindsay
Date:   2024.10.10

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This header contains the optional interface class for providing pluggable RNGs.
Some cryptographic and scientific programs might want tRNG and pRNG in the
  same build. Or they might want specific control over the nature of the RNG,
  rather than relying on the shared mystery implementation from the Platform's
  randomUInt32().
*/

#include "C3PRandom.h"
#include "../AbstractPlatform.h"

uint8_t C3PRandom::randomUInt8() {
  uint8_t ret = 0;
  fill((uint8_t*) &ret, (uint32_t) sizeof(uint8_t));
  return ret;
}

uint16_t C3PRandom::randomUInt16() {
  uint16_t ret = 0;
  fill((uint8_t*) &ret, (uint32_t) sizeof(uint16_t));
  return ret;
}

uint32_t C3PRandom::randomUInt32() {
  uint32_t ret = 0;
  fill((uint8_t*) &ret, (uint32_t) sizeof(uint32_t));
  return ret;
}

uint64_t C3PRandom::randomUInt64() {
  uint64_t ret = 0;
  fill((uint8_t*) &ret, (uint32_t) sizeof(uint64_t));
  return ret;
}

bool C3PRandom::randomBool() {
  return (0 == (0x80 & randomUInt8()));
}

// The floating point types will be artificially bounded to a lower entropy
//   than the type strictly allows. We do this to prevent tripping into such
//   special cases as NaN, and +/-Inf. The sign bit is preserved in all cases.
float C3PRandom::randomFloat() {
  // True entropy: 28-bit
  return (FLT_EPSILON * (int32_t) (0x87FFFFFF & randomUInt32()));
}

double C3PRandom::randomDouble() {
  // True entropy: 58-bit
  return (DBL_EPSILON * (int64_t) (0x87FFFFFFFFFFFFFF & randomUInt64()));
}


/*******************************************************************************
* Randomness (PRNG algo taken from pcg_basic C implementation)                 *
* https://github.com/imneme/pcg-c-basic                                        *
* https://www.pcg-random.org/                                                  *
*                                                                              *
* This is C3P's baseline RNG if the platform doesn't provide one. It may also  *
*   be used in conjunction with the platform's RNG to provide a pairing of the *
*   kind exemplified by /dev/random and /dev/urandom on a *nix system.         *
*******************************************************************************/
// Generate a uniformly distributed 32-bit random number
uint32_t C3P_pRNG::_pcg32_random_r() {
  uint64_t oldstate = _state;
  _state = oldstate * 6364136223846793005ULL + _inc;
  uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
  uint32_t rot = oldstate >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

void C3P_pRNG::_pcg32_srandom(uint64_t seed, uint64_t seq) {
  _state = 0U;
  _inc = ((seq) << 1u) | 1u;
  _pcg32_random_r();
  _state += seed;
  _pcg32_random_r();
}

/**
* Resets and seeds the pRNG.
*
* @param uint8_t* The buffer to fill.
* @param size_t The number of bytes to write to the buffer.
* @return 0, always.
*/
int8_t C3P_pRNG::init(const uint64_t SEED) {
  uint64_t safe_seed = SEED;
  if (0 == SEED) {
    // Seed the PRNG from the program start time.
    safe_seed = (uint64_t) micros() | (((uint64_t) micros()) << 32);
    sleep_ms(1);  // Incorporate jitter.
    safe_seed = safe_seed ^ ((((uint64_t) micros()) << 32) | (uint64_t) micros());
  }
  _pcg32_srandom(safe_seed, 7);
  return 0;
}

/**
* Fills the given buffer with random bytes.
* Blocks if there is nothing random available.
*
* @param uint8_t* The buffer to fill.
* @param size_t The number of bytes to write to the buffer.
* @return 0, always.
*/
int8_t C3P_pRNG::fill(uint8_t* buf, const uint32_t LEN) {
  int written_len = 0;
  while (4 <= (LEN - written_len)) {
    // If we have slots for them, just up-cast and write 4-at-a-time.
    *((uint32_t*) (buf + written_len)) = _pcg32_random_r();
    written_len += 4;
  }
  uint32_t slack = _pcg32_random_r();
  while (0 < (LEN - written_len)) {
    *(buf + written_len) = (uint8_t) 0xFF & slack;
    slack = slack >> 8;
    written_len++;
  }
  return 0;
}
