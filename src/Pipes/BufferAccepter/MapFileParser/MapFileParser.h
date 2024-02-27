/*
File:   MapFileParser.h
Author: J. Ian Lindsay
Date:   2023.12.29

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


A class to take raw text from a MAP file (optionally produced by ld), and parse
  it into fields of interest. If you don't immediately know what that means, you
  probably don't have any interest in this (entirely optional) class. It is
  intended for debugging software.

NOTE: This class is probably never going to be used in an environment that is
  memory-constrained or 32-bit. So all data and arithmetic is 64-bit for the
  sake of being able to effortlessly handle MAP files from 64-bit builds. On the
  expected platform, this will not cause a speed penalty, nor in inordinate
  memory burden (several megabytes, probably).

Constraints:
--------------------------------------------------------------------------------
  1) This class will not handle MAP files for builds that produced a binary
     larger than 2^32 bytes. That shouldn't be a problem, right?
*/

#ifndef __C3P_MAP_FILE_PARSER_SINK_H__
#define __C3P_MAP_FILE_PARSER_SINK_H__

#include "../BufferAccepter.h"

/* A class to represent a memory section. */
class MFP_MemSection {
  char* mem_region;
  uint64_t origin;
  uint64_t len;
  bool x_bit;
  bool r_bit;
  bool w_bit;
};

/* A class to represent something that takes up space in the binary. */
class MFP_Bytes {
  char* name;
  uint64_t origin;
  uint32_t len;
  uint32_t bin_offset;
  MFP_MemSection* section;
};

/* A class to represent a function. */
class MFP_Function : public MFP_Bytes {
};


/* A class to represent data. */
class MFP_Data : public MFP_Bytes {
};

/* A class to represent fill. */
class MFP_Fill : public MFP_Bytes {
};


enum class MFP_ParseStage : uint8_t {
  FRESH        = 0x00,  // Haven't seen the first byte.
  DISCARD      = 0x01,  // Parser is still in the discard listing.
  MEM_LAYOUT   = 0x02,  // The memory layout is being parsed.
  CONTENT      = 0x03,  // The real content of the build is being parsed.
  COMPLETE     = 0x04   // Parser has seen the end of the data it cares about.
};


/*
* The actual working parser object.
*/
class MapFileParser : public BufferAccepter {
  public:
    MapFileParser() {};
    ~MapFileParser() {};

    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    /* State-check accessors. */
    inline bool parseInProgress() {       return _parse_in_progress;     };
    inline bool parseComplete() {         return _parse_complete;        };

    int8_t  reset();


  private:
    uint32_t       _line_count        = 0;
    uint32_t       _byte_count        = 0;
    uint32_t       _discarded_count   = 0;
    uint32_t       _discarded_size    = 0;
    MFP_ParseStage _parser_stage      = MFP_ParseStage::FRESH;
    bool           _parse_in_progress = false;
    bool           _parse_complete    = false;
};

#endif  // __C3P_TEXT_LINE_CODEC_H__
