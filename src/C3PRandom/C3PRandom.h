/*
File:   C3PRandom.h
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

#ifndef __C3PRANDOM_H__
#define __C3PRANDOM_H__

#include "../CppPotpourri.h"


/*******************************************************************************
* Generic RNG interface.
*******************************************************************************/
class C3PRandom {
  public:
    bool     randomBool();
    uint8_t  randomUInt8();
    uint16_t randomUInt16();
    uint32_t randomUInt32();
    uint64_t randomUInt64();
    float    randomFloat();
    double   randomDouble();
    virtual int8_t fill(uint8_t*, const uint32_t LEN) =0;  // True implementation.
};


/*******************************************************************************
* A bundled pRNG based on pcg_basic. Creates deterministic bitstreams.
*******************************************************************************/
class C3P_pRNG : public C3PRandom {
  public:
    C3P_pRNG() : _state(0x853c49e6748fea9bULL), _inc(0xda3e39cb94b95bdbULL) {};
    ~C3P_pRNG() {  _state = 0;  _inc = 0;  };

    int8_t init(const uint64_t SEED = 0);
    int8_t fill(uint8_t*, const uint32_t LEN);


  private:
    uint64_t _state;  // RNG state.  All values are possible.
    uint64_t _inc;    // Controls which RNG sequence (stream) is selected. Must *always* be odd.

    uint32_t _pcg32_random_r();   // Generate a uniformly distributed 32-bit random number.
    void     _pcg32_srandom(uint64_t seed, uint64_t seq);
};

#endif // __C3PRANDOM_H__
