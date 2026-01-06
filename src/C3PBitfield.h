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

    ~C3PBitfield() {
      if (nullptr != _mem) {
        uint8_t* tmp = _mem;
        _mem = nullptr;
        free(tmp);
      }
    };

    bool bitValue(const uint32_t BIT_IDX) {
      if (allocated()) {
        const uint8_t MASK = (1 << (BIT_IDX & 7));
        return (0 < (*(_mem + _byte_idx(BIT_IDX)) & MASK));
      }
      return false;
    };

    void bitValue(const uint32_t BIT_IDX, const bool VAL) {
      if (allocated()) {
        const uint32_t BYTE_IDX = _byte_idx(BIT_IDX);
        const uint8_t  MASK     = (1 << (BIT_IDX & 7));
        const uint8_t  OLD_VAL  = *(_mem + BYTE_IDX);
        *(_mem + BYTE_IDX) = (OLD_VAL & ~MASK) | (VAL ? MASK : 0);
      }
    };


    // TODO:
    //uint32_t idxFirstSet();  // The index of the first bit that is set.
    //uint32_t idxFirstClear();  // The index of the first bit that is clear.
    //uint32_t totalSet();       // The total number of set bits.
    //uint32_t totalClear();  // The total number of cleared bits.


  private:
    const uint32_t BITS;
    uint8_t* _mem;

    inline const uint32_t _byte_idx(const uint32_t BIT_IDX) {
      return ((BIT_IDX >> 3) + ((BIT_IDX & 7)? 1:0));
    };


    // Lazy allocator. Inits all bits to zero.
    bool allocated() {
      if ((nullptr == _mem) && (BITS > 0)) {
        if (nullptr == _mem) {
          const uint32_t MEM_COST = (_byte_idx(BITS-1))+1;
          _mem = (uint8_t*) malloc(MEM_COST);
          if (nullptr != _mem) {
            memset(_mem, 0, MEM_COST);
          }
        }
      }
      return (nullptr != _mem);
    };
};


#endif // __DS_C3P_BITFIELD_H
