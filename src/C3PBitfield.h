/*
File:   C3PBitfield.h
Author: J. Ian Lindsay
Date:   2025.12.21

Copyright 2025 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


A class for efficient aggregation of a large number of bits.
*/
#ifndef __DS_C3P_BITFIELD_H
#define __DS_C3P_BITFIELD_H

#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>


class C3PBitfield {
  public:
    C3PBitfield(const uint32_t BIT_COUNT) : BITS(BIT_COUNT), _mem(nullptr) {};
    ~C3PBitfield();

    bool     bitValue(const uint32_t BIT_IDX);
    void     bitValue(const uint32_t BIT_IDX, const bool VAL);
    uint32_t idxFirstSet();    // The index of the first bit that is set.
    uint32_t idxFirstClear();  // The index of the first bit that is clear.
    uint32_t totalSet();       // The total number of set bits.
    uint32_t totalClear();     // The total number of cleared bits.


  private:
    const uint32_t BITS;
    uint8_t* _mem;

    inline uint32_t _byte_idx(const uint32_t BIT_IDX) const {  return (BIT_IDX >> 3);  };
    inline uint32_t _byte_count() const {   return ((BITS + 7) >> 3);   };

    bool allocated();   // Lazy allocator. Inits all bits to zero.
};





C3PBitfield::~C3PBitfield() {
  if (nullptr != _mem) {
    uint8_t* tmp = _mem;
    _mem = nullptr;
    free(tmp);
  }
}


bool C3PBitfield::allocated() {
  if ((nullptr == _mem) && (BITS > 0)) {
    if (nullptr == _mem) {
      const uint32_t MEM_COST = _byte_count();
      _mem = (uint8_t*) malloc(MEM_COST);
      if (nullptr != _mem) {
        memset(_mem, 0, MEM_COST);
      }
    }
  }
  return (nullptr != _mem);
}






bool C3PBitfield::bitValue(const uint32_t BIT_IDX) {
  if (allocated()) {
    const uint8_t MASK = (1 << (BIT_IDX & 7));
    return (0 < (*(_mem + _byte_idx(BIT_IDX)) & MASK));
  }
  return false;
}

void C3PBitfield::bitValue(const uint32_t BIT_IDX, const bool VAL) {
  if (allocated()) {
    const uint32_t BYTE_IDX = _byte_idx(BIT_IDX);
    const uint8_t  MASK     = (1 << (BIT_IDX & 7));
    const uint8_t  OLD_VAL  = *(_mem + BYTE_IDX);
    *(_mem + BYTE_IDX) = (OLD_VAL & ~MASK) | (VAL ? MASK : 0);
  }
}




// Add these inside the class (public section), replacing the TODOs:

uint32_t C3PBitfield::idxFirstSet() {     // index of first bit that is set
  if (!allocated()) {
    return UINT32_MAX;
  }

  const uint32_t BYTE_COUNT = ((BITS + 7) >> 3);
  const uint32_t FULL_BYTES = (BITS >> 3);
  const uint32_t TAIL_BITS  = (BITS & 7);

  for (uint32_t b = 0; b < BYTE_COUNT; b++) {
    uint8_t v = _mem[b];

    // Mask out unused bits in the last byte (they should not count as set).
    if ((b == FULL_BYTES) && (TAIL_BITS != 0)) {
      const uint8_t VALID_MASK = (uint8_t)((1u << TAIL_BITS) - 1u);
      v &= VALID_MASK;
    }

    if (v) {
      // find first set bit in v
      for (uint8_t bit = 0; bit < 8; bit++) {
        if (v & (1u << bit)) {
          const uint32_t idx = (b << 3) + bit;
          return (idx < BITS) ? idx : UINT32_MAX;
        }
      }
    }
  }

  return UINT32_MAX;
}


uint32_t C3PBitfield::idxFirstClear() {   // index of first bit that is clear
  if (!allocated()) {  return UINT32_MAX;  }

  const uint32_t BYTE_COUNT = ((BITS + 7) >> 3);
  const uint32_t FULL_BYTES = (BITS >> 3);
  const uint32_t TAIL_BITS  = (BITS & 7);

  for (uint32_t b = 0; b < BYTE_COUNT; b++) {
    uint8_t v = _mem[b];

    // For the last byte, force unused bits to 1 so they don't appear "clear".
    if ((b == FULL_BYTES) && (TAIL_BITS != 0)) {
      const uint8_t VALID_MASK = (uint8_t)((1u << TAIL_BITS) - 1u);
      v |= (uint8_t)~VALID_MASK;
    }

    if (v != 0xFF) {
      // find first clear bit in v
      for (uint8_t bit = 0; bit < 8; bit++) {
        if (!(v & (1u << bit))) {
          const uint32_t idx = (b << 3) + bit;
          return (idx < BITS) ? idx : UINT32_MAX;
        }
      }
    }
  }

  return UINT32_MAX;
}


/*
*
* @return Total number of set bits.
*/
uint32_t C3PBitfield::totalSet() {
  if (!allocated()) {  return 0;  }

  static const uint8_t POP4[16] = {  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4  };

  const uint32_t BYTE_COUNT = ((BITS + 7) >> 3);
  const uint32_t FULL_BYTES = (BITS >> 3);
  const uint32_t TAIL_BITS  = (BITS & 7);

  uint32_t total = 0;
  for (uint32_t b = 0; b < BYTE_COUNT; b++) {
    uint8_t v = _mem[b];

    // Mask out unused bits in the last byte.
    if ((b == FULL_BYTES) && (TAIL_BITS != 0)) {
      const uint8_t VALID_MASK = (uint8_t)((1u << TAIL_BITS) - 1u);
      v &= VALID_MASK;
    }

    total += POP4[v & 0x0F] + POP4[v >> 4];
  }

  return total;
}


/*
*
* @return Total number of cleared bits.
*/
uint32_t C3PBitfield::totalClear() {      // total number of cleared bits
  // Only count bits inside [0, BITS). Unallocated implies all-zero -> all clear.
  if (BITS == 0) {
    return 0;
  }
  if (!allocated()) {
    return BITS;
  }
  return (BITS - totalSet());
}

#endif // __DS_C3P_BITFIELD_H
