/*
File:   PerlinNoise.cpp
Author: J. Ian Lindsay
Date:   2025.06.26

Perlin noise generation.
*/

#include "C3PRandom.h"
#include "../AbstractPlatform.h"


/**
* Constructor
*/
PerlinNoise::PerlinNoise(uint32_t width,
                        uint32_t height,
                        float scale,
                        int octaves,
                        float persistence) :
  _t_w(width), _t_h(height),
  _seed(0),
  _offset_x(), _offset_y(0),
  _scale(scale), _octaves(octaves),
  _octave_freq(2.0f),
  _persistence(persistence),
  _field(nullptr) {}



PerlinNoise::~PerlinNoise() {
  if (nullptr != _field) {
    free(_field);
    _field = nullptr;
  }
}



/**
* Allocates memory, if necessary and prepares the class for use.
* This function can be used in the manner of a reset(). Memory will be zeroed,
*   but not reallocated.
* Calling init() with (SEED == 0) will re-use the existing seed (if non-zero),
*   or pull from the platform's RNG.
*/
int8_t PerlinNoise::init(const uint64_t SEED) {
  int8_t ret = -1;
  uint64_t prng_seed = SEED;
  if (0 == SEED) {
    prng_seed = (0 != _seed) ? _seed : random_fill((uint8_t*) &_seed, (uint32_t) sizeof(_seed));
  }
  _seed = prng_seed;

  if ((0 < _t_w) & (0 < _t_h)) {
    ret--;
    // Check for memory allocation, making one if necessary, and zeroing it.
    if (nullptr == _field) {
      // This is a 2-field.
      _field = (float*) malloc((_t_w * _t_h) * sizeof(float));
    }
    if (nullptr != _field) {
      ret--;
      // Zero the field.
      for (uint32_t i = 0; i < (_t_w * _t_h); ++i) {  _field[i] = 0.0f;  }
      if (0 == rng.init(_seed)) {
        reshuffle();  // From the seeded RNG, create the permutation vector.
        ret = 0;
      }
    }
  }
  return ret;
}


// TODO: Baked into this function are assumptions that are undesired:
//   1. The field density is neutral (0.5 average).
void PerlinNoise::reshuffle() {
  // Initialize permutation vector [0..255]
  for (uint16_t i = 0; i < 256; ++i) {    _perm[i] = (uint8_t) i;   }
  for (uint16_t i = 255; i > 0; --i) {  // Shuffle it...
    uint16_t j = (rng.randomUInt32() % i);
    uint8_t tmp = _perm[i];
    _perm[i] = _perm[j];
    _perm[j] = tmp;
  }
  // Duplicate to avoid overflow checks.
  for (uint16_t i = 0; i < 256; ++i) {   _perm[256 + i] = _perm[i];  }
}


int8_t PerlinNoise::setParameters(float scale, int octaves, float persistence, float freq) {
  // TODO: Error check.
  _scale = scale;
  _octaves = octaves;
  _persistence = persistence;
  _octave_freq = freq;
  return 0;
}


float PerlinNoise::_fade(float t) const {
  // 6t^5 - 15t^4 + 10t^3
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise::_lerp(float a, float b, float t) const {
  return (a + t * (b - a));
}

// Convert low 2 bits of hash into 4 possible gradient directions
float PerlinNoise::_grad(int hash, float x, float y) const {
  int   h = hash & 3;
  float u = (((h & 1) == 0) ? x : -x);
  float v = (((h & 2) == 0) ? y : -y);
  return (u + v);
}


int8_t PerlinNoise::apply() {
  int8_t ret = -1;
  if (nullptr != _field) {
    //const float MAXCHANNEL = 1.0f;   // Maximum channel value.
    for (uint32_t j = 0; j < _t_h; ++j) {
      for (uint32_t i = 0; i < _t_w; ++i) {
        const float x = (_offset_x + i) / _scale;
        const float y = (_offset_y + j) / _scale;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float noiseSum = 0.0f;
        float maxAmp = 0.0f;
        for (int o = 0; o < _octaves; ++o) {   // Sum multiple octaves
          const float xi = x * frequency;
          const float yi = y * frequency;
          const int X = static_cast<int>(std::floor(xi)) & 255;
          const int Y = static_cast<int>(std::floor(yi)) & 255;
          const float xf = xi - std::floor(xi);
          const float yf = yi - std::floor(yi);
          const float u = _fade(xf);
          const float v = _fade(yf);
          const int aa = _perm[X + _perm[Y]];
          const int ab = _perm[X + _perm[Y + 1]];
          const int ba = _perm[X + 1 + _perm[Y]];
          const int bb = _perm[X + 1 + _perm[Y + 1]];
          const float x1 = _lerp(_grad(aa, xf, yf),     _grad(ba, xf - 1, yf), u);
          const float x2 = _lerp(_grad(ab, xf, yf - 1), _grad(bb, xf - 1, yf - 1), u);
          const float val = _lerp(x1, x2, v);
          noiseSum  += val * amplitude;
          maxAmp    += amplitude;
          amplitude *= _persistence;
          frequency *= _octave_freq;
        }

        // Normalize to [-1,1], then to [0,1].
        // TODO? Then scale to bounds.
        const float NORMALIZED = (noiseSum / maxAmp + 1.0f) * 0.5f;
        *(_field + ((j * _t_w) + i)) = NORMALIZED;
        //*(_field + ((j * _t_w) + i)) = (float) std::round(NORMALIZED * MAXCHANNEL);
      }
    }
    ret = 0;
  }
  return ret;
}
