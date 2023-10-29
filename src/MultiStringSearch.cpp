/*
File:   MultiStringSearch.h
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
#include "MultiStringSearch.h"
#include "CppPotpourri.h"


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
  _last_match = nullptr;
  _last_full_match_offset = -1;
  _search_length = 0;
  _starting_offset = 0;
  _next_starting_offset   = -1;
  _needles_found = 0;
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
        _sdef_pool[_defs_added].enabled = true;
        _defs_added++;
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
  if (nullptr != _src) {                                 return -2;  }  // Search in progress.
  if ((0 > STARTING_OFFSET) | (0 >= SEARCH_LEN)) {       return -3;  }  // Params are confused.
  const int32_t INPUT_LENGTH = untrusted_src->length();
  if (INPUT_LENGTH < (STARTING_OFFSET + SEARCH_LEN)) {   return -3;  }  // Params are confused.
  reset();
  _src = untrusted_src;
  _search_length        = SEARCH_LEN;
  _starting_offset      = STARTING_OFFSET;
  _next_starting_offset = STARTING_OFFSET;
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
* TODO: A speed efficiency gain can be made by pushing the multiple-search
*   feature down into locate(). But that will take some careful planning.
*
* @return the number of search hits (after factoring out collision) on success.
*/
int MultiStringSearch::continueSearch() {
  int ret = 0;
  _last_match = nullptr;
  if (searchRunning()) {
    const int INPUT_LEN = _src->length();
    // If this passes, we are assured that at least needle is enabled.
    // Update our search bounds.
    _update_next_starting_offsets(_next_starting_offset);

    int locate_results[_defs_added] = {0, };
    int  matches_this_run = 0;
    int  longest_match    = 0;
    bool longest_match_is_partial = false;
    for (uint8_t i = 0; i < _defs_added; i++) {
      //printf("continue iteration (%d) \n", i);
      // For any searches still running, find the next occurance.
      if (_sdef_pool[i].enabled) {
        const int REMAINING_SEARCH_LEN = (INPUT_LEN - _sdef_pool[i].offset_start);
        const int NEEDLE_COMPARE_LEN   = strict_min(REMAINING_SEARCH_LEN, _sdef_pool[i].SEARCH_STR_LEN);
        locate_results[i] = _src->locate(_sdef_pool[i].SEARCH_STR, NEEDLE_COMPARE_LEN, _sdef_pool[i].offset_start);
        if (-1 < locate_results[i]) {
          // There was a match on a needle. Was it complete?
          const bool WAS_COMPLETE_MATCH = (NEEDLE_COMPARE_LEN == _sdef_pool[i].SEARCH_STR_LEN);
          if (WAS_COMPLETE_MATCH) {
            // If a needle had a complete match, we mark its def to reflect so.
            _sdef_pool[i].offset_start = locate_results[i];
            _sdef_pool[i].offset_end   = (locate_results[i] + _sdef_pool[i].SEARCH_STR_LEN);
            _needles_found++;
            //printf("(%d) Found complete match %d\n", i, locate_results[i]);
          }

          if (longest_match < locate_results[i]) {            // If this was at least the longest
            longest_match_is_partial = !WAS_COMPLETE_MATCH;   //   match so far, we set the
          }                                                   //   feasibility of more searching.
          else if (longest_match == locate_results[i]) {      // If it is tied for longest, we
            longest_match_is_partial |= !WAS_COMPLETE_MATCH;  //   may be done with the search.
          }
          longest_match = strict_max(longest_match, locate_results[i]);
          matches_this_run++;
        }
      }
      else {
        locate_results[i] = -1;
      }
    }

    // Did we have matches?
    //printf("matches_this_run = %d \t longest_match_is_partial = %c\n", matches_this_run, (longest_match_is_partial ? '1':'0'));
    if ((!longest_match_is_partial) & (0 < matches_this_run)) {
      // With all the results collected from each locate() on each possible
      //   substring, did anything come back positive for a match? Return the
      //   longest match that is also complete.
      for (uint8_t i = 0; i < _defs_added; i++) {
        if (-1 != locate_results[i]) {
          if (locate_results[i] == longest_match) {
            _sdef_pool[i].offset_end = (locate_results[i] + _sdef_pool[i].SEARCH_STR_LEN);
            _last_match = &_sdef_pool[i];
            _next_starting_offset = _sdef_pool[i].offset_end;
            _last_full_match_offset = locate_results[i];
            ret = 1;
          }
        }
      }
    }
    else {
      // If the longest match was also a partial match, or there were no matches
      //   on this run, the search is concluded because we don't want to
      //   unwittingly replace longer strings that are cut off with (possibly
      //   overlapping) substrings that are complete matches.
      _next_starting_offset = INPUT_LEN;
    }
  }
  return ret;
}


bool MultiStringSearch::searchRunning() {
  // If this is set, a positive search state must be refuted.
  bool ret = (nullptr != _src);
  if (ret) {
    // No active needle definitions would be a sufficient reason to consider
    //   the search complete.
    bool any_def_active = false;
    for (uint8_t i = 0; i < _defs_added; i++) {
      any_def_active |= _sdef_pool[i].enabled;
    }
    ret &= any_def_active;
  }
  const int REMAINING_SEARCH_LEN = (_search_length - (_next_starting_offset - _starting_offset));
  ret &= (0 < REMAINING_SEARCH_LEN);
  if (ret) {
    // Finally, if the search has exhausted its input length, it is complete,
    //   even if there are still active searches for needles.
    ret &= (_next_starting_offset < _src->length());
  }
  return ret;
}


/**
* If a needle was found, this function will be called to realign the search
*   boundaries to possibly exclude space that is unproductive to search.
*/
void MultiStringSearch::_update_next_starting_offsets(const int NEW_START) {
  for (uint8_t i = 0; i < _defs_added; i++) {
    if (_sdef_pool[i].enabled) {
      if (_sdef_pool[i].offset_start < NEW_START) {
        _sdef_pool[i].offset_start = NEW_START;
      }
      _sdef_pool[i].offset_end = -1;
    }
  }
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


/**
* Find the number of searches that are unresolved.
*
* @return the length unambiguously searched.
*
*/
int MultiStringSearch::unresolvedSearches() {
  int ret = 0;
  for (uint8_t i = 0; i < _defs_added; i++) {
    if (_sdef_pool[i].searchRunning()) {
      ret++;
    }
  }
  return ret;
}


void MultiStringSearch::printDebug(StringBuilder* text_return) {
  StringBuilder::styleHeader1(text_return, "MultiStringSearch");
  text_return->concatf("\tNeedle size range:   [%d, %d]\n", minNeedleLength(), maxNeedleLength());
  text_return->concatf("\tSearch_length:       %d\n", _search_length);
  text_return->concatf("\tStarting offset:     %d\n", _starting_offset);
  text_return->concatf("\tNext offset:         %d\n", _next_starting_offset);
  text_return->concatf("\tNeedles found:       %d\n", needlesFound());
  text_return->concatf("\tResolved length:     %d\n", resolvedLength());
  text_return->concatf("\tHas match:           %c\n\t", ((nullptr == lastMatch()) ? 'n':'y'));
  StringBuilder::styleHeader2(text_return, "Needles:");
  for (uint8_t i = 0; i < _defs_added; i++) {
    text_return->concatf("\t%u (%sabled)\n", i, (_sdef_pool[i].enabled ? "en":"dis"));
    text_return->concatf("\t  SEARCH_STR (%d bytes):\t", _sdef_pool[i].SEARCH_STR_LEN);
    StringBuilder ascii_dump(_sdef_pool[i].SEARCH_STR, _sdef_pool[i].SEARCH_STR_LEN);
    ascii_dump.printDebug(text_return);
    text_return->concatf("\t  offset_start/end:\t(%d / %d)\n", _sdef_pool[i].offset_start, _sdef_pool[i].offset_end);
  }
}
