/*
File:   MapFileParser.cpp
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
  it into fields of interest.
*/

#include "MapFileParser.h"



  // "Memory Configuration"
  //RamExec          0x000000001000ec00 0x0000000000011400 xrw

  // "Linker script and memory map"


/*******************************************************************************
* Implementation of BufferAccepter
*******************************************************************************/

int8_t MapFileParser::pushBuffer(StringBuilder* buf) {
  if ((nullptr == buf) | (nullptr == _efferant)) {  return -1;  }  // Bailout.
  int8_t ret = -1;
  switch (_parser_stage) {
    case MFP_ParseStage::FRESH:
      // "Discarded input sections"
      break;

    case MFP_ParseStage::DISCARD:      break;
    case MFP_ParseStage::MEM_LAYOUT:   break;
    case MFP_ParseStage::CONTENT:      break;

    case MFP_ParseStage::COMPLETE:
      break;
    default:
      break;
  }
  return ret;
}


// NOTE: We have _some_ limit. But because we are acting as a sink, and the
//   memory-usage transform is unknowable without the actual input to consider,
//   we will always report a constant arbitrary value.
int32_t MapFileParser::bufferAvailable() {
  return 4096;
}



/*******************************************************************************
* MapFileParser specifics
*******************************************************************************/
