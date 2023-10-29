/*
File:   MultiStringSearch.cpp
Author: J. Ian Lindsay
Date:   2023.08.25

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
*/

#ifndef __C3P_TEXT_MULTISEARCH_H
#define __C3P_TEXT_MULTISEARCH_H

#include "StringBuilder.h"


/*******************************************************************************
* Search utility.
* This is concealed machinery that is used to contain some of the complexity
*   associated with searching for many strings at once. Search is greedy, so
*   the longest matching term is returned if searches mutually overlap.
* You probably don't want to instance these. Or maybe you do. If they are useful
*   beyond this context, consider them for promotion.
*******************************************************************************/

/*
* This class holds the definition and state for a single search term in a
*   multi-term concurrent search.
* NOTE: This class is strictly a state and logic container, and should have no
*   detailed inner workings. All the heavy-lifting is done by
*   StringBuilder::locate() calls made outside of this class.
*/
class StrSearchDef {
  public:
    const uint8_t* SEARCH_STR;
    const int      SEARCH_STR_LEN;   // String and length to search on.
    int            offset_start;     // If (>= 0), a match starts at this offset.
    int            offset_end;       // If (> offset_start), a match ends at this offset.
    bool           enabled;          //

    StrSearchDef(const uint8_t* BUF, const int LEN) :
      SEARCH_STR(BUF), SEARCH_STR_LEN(LEN), offset_start(-1), offset_end(-1), enabled(false) {};
    ~StrSearchDef() {};

    void reset();
    inline bool searchRunning() {  return (enabled & (0 <= offset_start) & (offset_start > offset_end));  };
    inline bool searchHit() {      return ((0 <= offset_start) & (0 <= offset_end));            };
};


/*
* Instance this class to conduct a search.
*/
class MultiStringSearch {
  public:
    MultiStringSearch(const uint8_t MAX_PARALLEL) :
      _MAX_SEARCH_TERMS(MAX_PARALLEL), _defs_added(0),
      _sdef_pool(nullptr), _src(nullptr),
      _search_length(0), _starting_offset(-1), _last_match(nullptr),
      _last_full_match_offset(-1), _next_starting_offset(-1), _needles_found(0) {};

    ~MultiStringSearch();

    bool   initialized();   // Allocates once on-demand.
    void   reset();
    int8_t addSearchTerm(const uint8_t* BUF, const int LEN);
    int    runSearch(StringBuilder* src, const int SEARCH_LEN, const int STARTING_OFFSET = 0);
    int    continueSearch();
    bool   searchRunning();
    int    resolvedLength();
    int    minNeedleLength();
    int    maxNeedleLength();
    int    unresolvedSearches();
    inline uint8_t       needlesDefined() {   return _defs_added;        };
    inline uint32_t      needlesFound() {     return _needles_found;     };
    inline StrSearchDef* lastMatch() {        return _last_match;        };

    void   printDebug(StringBuilder*);


  private:
    const uint8_t  _MAX_SEARCH_TERMS;     // How many parallel searches to run?
    uint8_t        _defs_added;
    StrSearchDef*  _sdef_pool;
    StringBuilder* _src;
    int            _search_length;
    int            _starting_offset;
    StrSearchDef*  _last_match;
    int            _last_full_match_offset;
    int            _next_starting_offset;
    uint32_t       _needles_found;   // How many needles have been found so far?

    void _update_next_starting_offsets(const int);
};

#endif  // __C3P_TEXT_MULTISEARCH_H
