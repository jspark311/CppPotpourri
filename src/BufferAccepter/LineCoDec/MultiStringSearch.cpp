/*
File:   MultiStringSearch.cpp
Author: J. Ian Lindsay
Date:   2023.09.12

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


A utility class that orchestrates concurrent search of a haystack for many
  needles.

TODO: Check that we aren't doing replacement at the trailing edge if there is
  a chance that the search will miss a multi-byte sequence. Might-shouldn't
  use StringBuilder::replace()...
  Can't easily leverage the tokenizer, either, since some termination
  sequences are multi-byte, and multiple empty lines will be crushed into a
  single line-break.
*/

#include <new>
#include "LineCoDec.h"


/*******************************************************************************
* StrSearchDef
*******************************************************************************/
/*
* Reset tracking data for this term.
*/
void StrSearchDef::reset() {
  offset_start = -1;
  offset_end   = -1;
  enabled      = true;
}


/*******************************************************************************
* MultiStringSearch
*******************************************************************************/

/*
*/
MultiStringSearch::~MultiStringSearch() {
  if (nullptr != _sdef_pool) {
    free(_sdef_pool);
    _sdef_pool = nullptr;
  }
}


/*
*/
bool MultiStringSearch::initialized() {
  if (nullptr == _sdef_pool) {
    _sdef_pool = (StrSearchDef*) malloc(sizeof(StrSearchDef) * _MAX_SEARCH_TERMS);
  }
  return (nullptr != _sdef_pool);
}


/*
*/
void MultiStringSearch::reset() {
  _src = nullptr;
  if (initialized()) {
    for (uint8_t i = 0; i < _defs_added; i++) {
      _sdef_pool[i].reset();
      _sdef_pool[i].enabled = true;
    }
  }
}


/**
*
* @return 0 on success
*        -1 on bad parameters.
*        -2 if the number of search terms is already at maximum.
*        -3 on failure to allocate.
*/
int8_t MultiStringSearch::addSearchTerm(const uint8_t* BUF, const int LEN) {
  int8_t ret = -1;
  if ((nullptr != BUF) & (0 < LEN)) {
    ret--;
    if (_defs_added < _MAX_SEARCH_TERMS) {
      ret--;
      if (initialized()) {
        new(_sdef_pool + _defs_added) StrSearchDef(BUF, LEN);  // Placement new.
        _sdef_pool[_defs_added++].enabled = true;;
        ret = 0;
      }
    }
  }
  return ret;
}


/**
* Begins the search on the given subject string. This function calls
*   continueSearch() for its success-case return value.
* Subsequent calls to continueSearch() will pick up the search where it left
*   off.
*
* Following a call to this function, there are three ways that class state
*   will be mutated:
*     1) Calling continueSearch()
*     2) Calling reset()
*     3) Destruction
*
* @param src is the subject of the search.
* @param SEARCH_LEN defines the length of the search.
* @param STARTING_OFFSET defines where the search begins.
* @return the number of search hits (after factoring out collision) on success
*        -1 on null paramater, or allocation failure.
*        -2 if search is already in progress.
*        -3 on nonsensical parameters such as (length < offset)
*/
int MultiStringSearch::runSearch(StringBuilder* untrusted_src, const int SEARCH_LEN, const int STARTING_OFFSET) {
  // Bailout clauses...
  if ((nullptr == untrusted_src) || (!initialized())) {  return -1;  }  // NOTE: short-circuit.
  if (nullptr != _src) {       return -2;  }  // Search in progress.
  const int32_t INPUT_LENGTH = untrusted_src->length();   // Find input bounds.
  if (INPUT_LENGTH <= STARTING_OFFSET) {                 return -3;  }  // Params are confused.
  reset();
  _src = untrusted_src;
  return continueSearch();
}


/**
* Runs the search iteratively until any of these conditions become true:
*   1) A match is resolved on one (and only one) needle.
*   2) The subject string is exhausted.
*   3) The search is aborted by calling reset().
*
* Search will iterate byte-wise inside of this stack frame until either
*   condition (1) or (2) is met, and will return 0 in either case. A return
*   value of 0 should be construed as an indication that the search is over.
*
* Under expected use, the caller would repeatedly call continueSearch() until
*   it, or the preceding call to runSearch(), returned 0. What the caller
*   does with mid-search state information is up to the caller, but the caller
*   should not mutate the subject, nor free memory used in the definitions of
*   the search (which doesn't copy anything) until the search is concluded for
*   one of the above reasons.
*
* @return the number of search hits (after factoring out collision) on success.
*/
int MultiStringSearch::continueSearch() {
  int ret = 0;
  if (nullptr != _src) {
  }
  if (initialized()) {
    for (uint8_t i = 0; i < _defs_added; i++) {
      _sdef_pool[i].reset();
      _sdef_pool[i].enabled = true;
    }
  }
  return ret;
}



/**
* Find the length of the original subject that was searched unambiguously.
* For a collection of single-byte search terms, this function would always
*   return the original length of the search subject.
* Typically, this would be called after a completed search, but it should be
*   accurate at any point.
*
* @return the length unambiguously searched.
*
*/
int MultiStringSearch::resolvedLength() {
  int ret = 0;
  if (nullptr != _src) {
    const int32_t INPUT_LENGTH = _src->length();   // Find input bounds.
    int downward_revision = 0;
    for (uint8_t i = 0; i < _defs_added; i++) {
      if (_sdef_pool[i].searchRunning()) {
        // Adjust downward to account for needles that are still unresolved.
        const int TEMP_OFFSET = _sdef_pool[i].offset_start;
        downward_revision = (0 == downward_revision) ? TEMP_OFFSET : strict_min(downward_revision, TEMP_OFFSET);
      }
    }
    // Rearrange the term to better suit our naming and preferred algebra...
    downward_revision = (INPUT_LENGTH - downward_revision);
    ret = downward_revision;
  }
  return ret;
}


int MultiStringSearch::minNeedleLength() {
  int ret = (_defs_added > 0) ? _sdef_pool[0].SEARCH_STR_LEN : 0;
  for (uint8_t i = 1; i < _defs_added; i++) {
    ret = strict_min(ret, _sdef_pool[i].SEARCH_STR_LEN);
  }
  return ret;
}


int MultiStringSearch::maxNeedleLength() {
  int ret = 0;
  for (uint8_t i = 0; i < _defs_added; i++) {
    ret = strict_max(ret, _sdef_pool[i].SEARCH_STR_LEN);
  }
  return ret;
}


void MultiStringSearch::printDebug(StringBuilder* text_return) {
  StringBuilder::styleHeader1(text_return, "MultiStringSearch");
  text_return->concatf("\tNeedle size range:   [%d, %d]\n", minNeedleLength(), maxNeedleLength());
  text_return->concatf("\tResolved length:     %d\n\t", resolvedLength());
  StringBuilder::styleHeader2(text_return, "Needles:");
  for (uint8_t i = 0; i < _defs_added; i++) {
    text_return->concatf("\t%u (%sabled)\n", i, (_sdef_pool[i].enabled ? "en":"dis"));
    text_return->concatf("\t  SEARCH_STR (%d bytes):\t", _sdef_pool[i].SEARCH_STR_LEN);
    StringBuilder ascii_dump(_sdef_pool[i].SEARCH_STR, _sdef_pool[i].SEARCH_STR_LEN);
    ascii_dump.printDebug(text_return);
    text_return->concatf("\t  offset_start/end:     \t(%d / %d)\n", _sdef_pool[i].offset_start, _sdef_pool[i].offset_end);
  }
}


// int8_t ::pushBuffer(StringBuilder* buf) {
//   if (0 != SEARCH_MASK) {
//     // If the conversion process would change the length of the string, we will
//     //   need to reallocate/copy. But don't do that just yet. Do the search and
//     //   calculate the new size and boundary rules to make sure we don't do the
//     //   replacement for nothing (since it may happen in-situ).
//     const int LENGTH_DIFFERENTIAL = (LT_LEN_FINAL + lt_len_max_initial);
//     const uint8_t* INPUT_BUFFER = buf->string();
//     int32_t lt_search_lit[MAX_SEARCHES]  = {0, };  // Search-tracking
//     const uint8_t* seg_start_pos = INPUT_BUFFER;
//     int32_t search_idx    = 0;
//     bool keep_searching = true;
//     int searches_unresolved = 0;
//
//     while (keep_searching) {
//       const int32_t CURRENT_SEARCH_LENGTH = lt_len_initial[n];
//       bool found_terminator = false;
//       const char INPUT_BYTE      = *(INPUT_BUFFER + i);
//       for (uint8_t i = 0; i < MAX_SEARCHES; i++) {
//         if (0 < lt_len_initial[i]) {  // We are searching for a terminator of this length.
//           const char* TERMINATOR_STR = lineTerminatorLiteralStr();
//           if (0 < lt_search_lit[i]) {
//             // We are in the middle of this terminator.
//             const int TERM_STR_OFFSET = (search_idx - lt_len_initial[i]);
//             if (INPUT_BYTE == *(TERMINATOR_STR + TERM_STR_OFFSET)) {
//               if ((SEARCH_OFFSET + 1) == lt_len_initial[i]) {
//                 found_terminator = true;
//                 searches_unresolved--;
//               }
//             }
//             else {
//               searches_unresolved--;
//             }
//           }
//           else if (INPUT_BYTE == *TERMINATOR_STR) {
//             // This is the first character of the search term.
//             lt_len_initial[i] = search_idx;
//             if (1 == lt_len_initial[i]) {
//               // It is also the only byte.
//               found_terminator = true;
//             }
//             else {
//               searches_unresolved++;
//             }
//           }
//         }
//       }
//
//
//       if (found_terminator) {             // If any terminal sequence was found...
//         if (0 == searches_unresolved) {   // ...and there are no unresolved searches...
//           const int SEG_LENGTH = ((INPUT_BUFFER + i) - seg_start_pos) - lt_len_initial[i];
//           if (0 < SEG_LENGTH) {
//             push_buf.concat(seg_start_pos, SEG_LENGTH);
//           }
//           seg_start_pos = (INPUT_BUFFER + i);
//         }
//       }
//
//       search_idx++;
//       keep_searching = (PURE_TAKE_LENGTH > search_idx);
//     }
//   }
// }
