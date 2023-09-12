/*
File:   LineCoDec.cpp
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


/*******************************************************************************
* Implementation of BufferAccepter, via CoDec.
*******************************************************************************/

int8_t LineEndingCoDec::pushBuffer(StringBuilder* buf) {
  if ((nullptr == buf) | (nullptr == _efferant)) {  return -1;  }  // Bailout.
  int8_t ret = -1;

  const int32_t INPUT_LENGTH    = buf->length();                 // Find input bounds.
  const int32_t MAX_PUSH_LENGTH = _efferant->bufferAvailable();  // Find efferant push bounds.
  // Find the pure-take bounds (the amount we can take if we were not chunking or replacing).
  const int32_t PURE_TAKE_LENGTH = strict_min(INPUT_LENGTH, MAX_PUSH_LENGTH);
  if (PURE_TAKE_LENGTH > 0) {
    // If there is no line terminator specified, we can not chunk, and we
    //   should just forward pushed buffers with any search terms removed.
    // NOTE: LineTerm::INVALID has a length of zero.
    // NOTE: LineTerm::ZEROBYTE has a length of one, and this constitutes
    //   special handling.
    const uint8_t LT_LEN_FINAL = (LineTerm::ZEROBYTE != _term_seq) ? lineTerminatorLength(_term_seq) : 0;

    // We abstract the assignment of the forwarded buffer from the from source,
    //   despite it being assigned that way by default. If mutation is required,
    //   buf_to_push will be assigned to mutation_buf instead. This prevents us
    //   having to shuffle and copy needlessly if we aren't doing replacement.
    StringBuilder  mutation_buf;
    StringBuilder* buf_to_push = buf;
    bool push_mutated_buf = false;
    int length_taken = 0;

    // We don't bother replacing if there will be no change. But if the
    //   replacement_mask contains something other-than our desired LineTerm,
    //   we prepare for a search.
    const uint8_t TARGET_TERM_MASK = (1 << (uint8_t) _term_seq);
    const uint8_t SEARCH_MASK      = (_replacement_mask & ~(TARGET_TERM_MASK));
    // Do we need to do anything for search and relace? Set up for it, if so.
    if (0 != SEARCH_MASK) {
      const uint8_t MAX_SEARCHES = (uint8_t) LineTerm::INVALID;
      MultiStringSearch search_machine(MAX_SEARCHES);

      for (uint8_t i = 0; i < MAX_SEARCHES; i++) {
        if ((1 << i) & SEARCH_MASK) {
          // Record information for terminators included in the search.
          const LineTerm TMP_TERM = (LineTerm) i;
          int8_t ret_st_add = search_machine.addSearchTerm(
            (const uint8_t*) lineTerminatorLiteralStr(TMP_TERM),
            lineTerminatorLength(TMP_TERM)
          );
          // If we don't have enough memory to define a termination sequences, we
          //   certainly don't have enough to do a replace(). Bail out.
          if (0 != ret_st_add) {  return ret;  }
        }
      }

      int search_result = search_machine.runSearch(buf, PURE_TAKE_LENGTH);
      // We specially gate-keep the inside loop to isolate checks for errors
      //   that are applicable to runSearch(), but not continueSearch(), which
      //   never returns a failure.
      if (0 <= search_result) {
        bool keep_searching = (0 < search_result);
        while (keep_searching) {
          // We had a hit from the search. We are now assured that we will be
          //   pushing the mutated buffer. Make the assignments.
          if (!push_mutated_buf) {
            push_mutated_buf = true;
            buf_to_push = buf;
          }

          // Find the needle that matched, and derive some copy parameters.
          StrSearchDef* match_def = search_machine.lastMatch();
          const int RETAIN_START_OFFSET = length_taken;
          const int RETAIN_STOP_OFFSET  = match_def->offset_start;
          const int RETAIN_LENGTH       = (RETAIN_STOP_OFFSET - RETAIN_START_OFFSET);

          // If adding the new chunk to the mutated buffer would exceed
          //   our length limit, cut off the search.
          if ((RETAIN_LENGTH + LT_LEN_FINAL) >= MAX_PUSH_LENGTH) {
            // Copy out the new chunk of the string with the matching needle
            //   removed, and the desired line terminator put in its place.
            StringBuilder stack_obj;
            stack_obj.concatLimit(buf, RETAIN_LENGTH);
            if (LT_LEN_FINAL > 0) {
              stack_obj.concat((const uint8_t*) lineTerminatorLiteralStr(_term_seq), LT_LEN_FINAL);
              stack_obj.string();  // Collapse ahead of handoff.
            }

            length_taken += (RETAIN_LENGTH + match_def->SEARCH_STR_LEN);
            buf_to_push->concatHandoff(&stack_obj);
            // Try to continue the search.
            keep_searching = (0 < search_machine.continueSearch());
          }
          else {
            keep_searching = false;
          }
        }
      }
      else {
        // A failure to initiate the search is a halting failure. Bail out.
        return ret;
      }
      //StringBuilder log;
      //search_machine.printDebug(&log);
      //printf("\n%s\n", (char*) log.string());

      // If we made it this far without a bailout, it means that search and
      //   replace went well, and we should cull the source string according
      //   to how much of the input we took.
      // We must be done with the search results beyond this point.
      buf->cull(length_taken);
    }

    // With search and replace optionally completed, we can push the result
    //   according to class settings.
    if (holdUntilBreak()) {
      // Chunking will complicate our lives, slightly.
      ret = _push_with_callbreak(buf_to_push);  // TODO: Wrong
    }
    else {
      // Without chunking, we don't need to do anything special. Just forward
      //   everything we presently have that is certain.
      switch (_efferant->pushBuffer(buf_to_push)) {
        case -1:
        case 0:
          // We planned very hard to avoid this case. Yet here we are. If
          //   mutated buffer was unclaimed by the efferant, prepend it back
          //   onto the source buffer.
          // TODO: This is technically a violation of contract. Clarify desired behavior.
          if (push_mutated_buf) {
            buf->prependHandoff(buf_to_push);
          }
          break;

        default:
          break;
      }
    }

    ret = (((INPUT_LENGTH - buf->length()) < INPUT_LENGTH) ? 0 : 1);
  }

  return ret;
}


int8_t LineEndingCoDec::_push_no_callbreak(StringBuilder* buf) {
  int8_t ret = -1;
  return ret;
}


/*
* NOTE: Private method, but follows the same return conventions and rules as
*   does pushBuffer() itself.
* TODO: Iterate efferent push if isometry is desired.
*/
int8_t LineEndingCoDec::_push_with_callbreak(StringBuilder* buf) {
  int8_t ret = -1;
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
  const uint8_t TARGET_TERM_MASK = (1 << (uint8_t) r_term);
  if (replace) {  _replacement_mask |= TARGET_TERM_MASK;     }
  else {          _replacement_mask &= ~(TARGET_TERM_MASK);  }
}


bool LineEndingCoDec::replaceOccurrencesOf(LineTerm r_term) {
  const uint8_t TARGET_TERM_MASK = (1 << (uint8_t) r_term);
  const uint8_t SEARCH_MASK      = (_replacement_mask & ~(TARGET_TERM_MASK));
  return (0 != SEARCH_MASK);
}


void LineEndingCoDec::holdUntilBreak(bool x) {
  _hold_until_break = x;
  if (!x) {  _isometric_call_to_break = false;  }
}

void LineEndingCoDec::isometricCallAndBreak(bool x) {
  _isometric_call_to_break = x;
}
