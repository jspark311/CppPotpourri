/*
File:   LineCoDec.h
Author: J. Ian Lindsay
Date:   2023.08.19

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


A text-converter that unifies line-endings. Usually in preparation for
  rendering printable text to some medium.
*/

#include "LineCoDec.h"


// TODO: Check that we aren't doing replacement at the trailing edge if there is
//   a chance that the search will miss a multi-byte sequence. Might-shouldn't
//   use StringBuilder::replace()...
//   Can't easily leverage the tokenizer, either, since some termination
//   sequences are multi-byte, and multiple empty lines will be crushed into a
//   single line-break.
int8_t LineEndingCoDec::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;

  // Do we need to do anything?
  const uint16_t TARGET_TERM_MASK = (1 << (uint8_t) _term_seq);
  const uint16_t SEARCH_MASK      = (_replacement_mask & ~(TARGET_TERM_MASK));
  const uint8_t  MAX_SEARCHES     = (uint8_t) LineTerm::INVALID;

  if (0 != SEARCH_MASK) {
    // If the replacement_mask contains something other-than our desired
    //   LineTerm (we don't bother replacing if there will be no change),
    //   we prepare for search.
    // We won't try to use StringBuilder::replace(), due to the parallel nature
    //   of the task. The logic of the problem is simpler if we don't try to
    //   break it up.
    // Find the byte differential, if any.
    uint8_t lt_len_final       = lineTerminatorLength(_term_seq);
    uint8_t lt_len_max_initial = 0;
    uint8_t lt_len_initial[MAX_SEARCHES] = {0, };
    int32_t lt_search_lit[MAX_SEARCHES]  = {0, };

    for (uint8_t i = 0; i < MAX_SEARCHES; i++) {
      const uint16_t CURRENT_MASK_BIT = (1 << i);
      if (CURRENT_MASK_BIT & SEARCH_MASK) {
        // Record length information for terminators for which we will search.
        lt_len_initial[i]  = lineTerminatorLength((LineTerm) i);
        lt_len_max_initial = strict_max(lt_len_max_initial, lt_len_initial[i]);
      }
    }
    const bool LENGTH_DIFFERENTIAL = (lt_len_max_initial != lt_len_final);
    // If the conversion process would change the length of the string, we will
    //   need to reallocate/copy. But don't do that just yet. Do the search and
    //   calculate the new size and boundary rules to make sure we don't do the
    //   replacement for nothing (since it may happen in-situ).
    const uint8_t* INPUT_BUFFER = buf->string();
    const int32_t  INPUT_LENGTH = buf->length();
    int32_t replacable_lt_count = 0;

    for (int32_t n = 0; n < INPUT_LENGTH; n++) {
      const int32_t CURRENT_SEARCH_LENGTH = lt_len_initial[n];
      for (uint8_t i = 0; i < MAX_SEARCHES; i++) {
        switch (lt_len_initial[i]) {
          case 0:  break;   // No search.
          case 1:
            break;
          default:
            if (0 < lt_search_lit[i]) {
              // We are in the middle of this terminator.
            }
            else {
            }
            break;
        }
      }

      //*(INPUT_BUFFER + i)
      //if () {
      //}
    }
    int32_t total_len_change = replacable_lt_count * LENGTH_DIFFERENTIAL;


    if (nullptr != _efferant) {
      // TODO: There may be a smarter way to do this with less branching, and
      //   without calling StringBuilder::replace().
      switch (_term_seq) {
        case LineTerm::ZEROBYTE:  // "\0"
          // TODO: This might be special... Perverse consequence warning.
          //   Write the unit tests for this before trying to implement it.
          ret = _efferant->pushBuffer(buf);
          break;
        case LineTerm::CR:
          if (replaceOccurrencesOf(LineTerm::CRLF)) {  buf->replace("\r\n", "\r");  }
          if (replaceOccurrencesOf(LineTerm::LF)) {    buf->replace("\n", "\r");    }
          ret = _efferant->pushBuffer(buf);
          break;
        case LineTerm::LF:
          if (replaceOccurrencesOf(LineTerm::CRLF)) {  buf->replace("\r\n", "\n");  }
          if (replaceOccurrencesOf(LineTerm::CR)) {    buf->replace("\r", "\n");    } 
          ret = _efferant->pushBuffer(buf);
          break;
        case LineTerm::CRLF:
          // TODO: This won't work because the end result will be things like "\r\r\n".
          //   More reason to not try to use StringBuilder::replace()...
          //buf->replace("\r",   "\r\n");
          //buf->replace("\n",   "\r\n");
          ret = _efferant->pushBuffer(buf);
          break;
      }
    }
  }



  if (!holdUntilBreak()) {
    // Without chunking, we don't need to do anything special. Just forward
    //   everything we presently have that is certain.
  }
  else {
    // Chunking will complicate our lives, slightly.
  }
  return ret;
}


// NOTE: This function will over-report if doing a conversion that
//   increases the byte count versus the input, and it will under-report if
//   conversion decreases the count. This is perfectly acceptable behavior if
//   the results of return values are observed within contractual limits.
int32_t LineEndingCoDec::bufferAvailable() {
  return ((nullptr != _efferant) ? _efferant->bufferAvailable() : 0);
}


/*******************************************************************************
* LineEndingCoDec specifics
*******************************************************************************/

void LineEndingCoDec::replaceOccurrencesOf(LineTerm r_term, bool replace) {
  const uint16_t TARGET_TERM_MASK = (1 << (uint8_t) r_term);
  if (replace) {  _replacement_mask |= TARGET_TERM_MASK;     }
  else {          _replacement_mask &= ~(TARGET_TERM_MASK);  }
}


bool LineEndingCoDec::replaceOccurrencesOf(LineTerm r_term) {
  const uint16_t TARGET_TERM_MASK = (1 << (uint8_t) r_term);
  const uint16_t SEARCH_MASK      = (_replacement_mask & ~(TARGET_TERM_MASK));
  return (0 != SEARCH_MASK);
}


void LineEndingCoDec::holdUntilBreak(bool x) {
  _hold_until_break = x;
  if (!x) {  _isometric_call_to_break = false;  }
}

void LineEndingCoDec::isometricCallAndBreak(bool x) {
  _isometric_call_to_break = x;
}
