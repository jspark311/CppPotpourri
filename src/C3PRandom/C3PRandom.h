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
* A bundled pRNG based on pcg_basic. Creates deterministic bitstreams.         *
* PRNG algo and initial parameters taken from pcg_basic C implementation:      *
*          https://github.com/imneme/pcg-c-basic                               *
*          https://www.pcg-random.org/                                         *
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


/*******************************************************************************
* Useful agorithms that use randomness as an input term.                       *
*******************************************************************************/

/*
* Generates a field of Perlin noise with the given parameters.
*
* TODO: Allocates heap. But should also work like TimeSeries and other memory-heavy
*   classes to be constructable with a pointer reference to an externally-allocated
*   memory range.
*/
class PerlinNoise {
  public:
    C3P_pRNG rng;   // TODO: Making this public is risky. Undefined behavior at mercy of caller.

    /**
     * @param width     Width of the result field.
     * @param height    Height of the result field.
     * @param scale     The "zoom" of the noise (larger values for broader scale).
     * @param octaves   Number of octaves to sum (controls detail).
     * @param persistence  Amplitude falloff per octave.
     */
    PerlinNoise(uint32_t width,
                uint32_t height,
                float scale       = 1.0f,
                int octaves       = 1,
                float persistence = 0.5f);
    ~PerlinNoise();

    int8_t init(const uint64_t SEED = 0);   // Allocates memory for the result field.

    // Change or set the parameters mid life-cycle.
    // NOTE: Size cannot be changed, due to memory implications.
    int8_t setParameters(float scale, int octaves, float persistence, float freq = 2.0f);


    inline void setOffset(uint32_t x, uint32_t y) {
      _offset_x = x; _offset_y = y;
    };
    inline void getOffset(uint32_t* x, uint32_t* y) {
      *x = _offset_x; *y = _offset_y;
    };

    inline float valueAtPoint(uint32_t i, uint32_t j) {
      return (
        (nullptr == _field) ? 0.0f : *(_field + ((j * _t_w) + i))
      );
    };

    void   reshuffle();   // Re-create the permutation vector.
    int8_t apply();       // Generate the field.



  private:
    const uint32_t _t_w;
    const uint32_t _t_h;
    uint64_t  _seed;
    uint32_t  _offset_x;
    uint32_t  _offset_y;
    float     _scale;
    int       _octaves;
    float     _octave_freq;
    float     _persistence;
    float*    _field;
    uint8_t   _perm[512] = {0};  // 512-length permutation vector

    float _fade(float t) const;
    float _lerp(float a, float b, float t) const;
    float _grad(int hash, float x, float y) const;

};



#endif // __C3PRANDOM_H__
