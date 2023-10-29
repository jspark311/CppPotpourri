/*
File:   LineCoDec.h
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


A class to enforce conformity and grouping of line-endings. It has two
independent and orthogonal concerns, either of which are optional.


Optional feature: terminal sequence transform
--------------------------------------------------------------------------------
This class is the gateway between definitions of what defines a "line" of text
  for internal firmware versus any external system. It ought to handle all line
  endings knowable by the firmware at build time. These are defined by the
  LineTerm enum in EnumeratedTypeCodes.h. The underpinnings of this feature is
  a bounded search-and-replace.

Special semantics surrounding ZEROBYTE:
ZEROBYTE is taken to be a C-string, and is used by this class as a "disregard"
  value for the field it is used with.
If sequences other than ZEROBYTE are specified for search, and the replacement
  value is set to ZEROBYTE, the result will the input buffer tokenized by the
  search sequences, those sequences will be removed, and the buffer dispatched
  according to the

Behavior of LineEndingCoDec::pushBuffer():
  If required, a terminator replacement will be done to the extent that the
  resulting transformed buffer can fit in the downstream space constraint. If
  a multibyte sequence is specified for search, the last bytes of the buffer
  will be rejected if they produce an unresolved match. So be careful with those
  CRLF systems...


Optional feature: call-breaking
--------------------------------------------------------------------------------
Definition of "call-break": A call to efferant->pushBuffer() that is
  strategically made to coincide with line termination sequences at the ends of
  the buffers pushed. Usually for the sake of offloading implementation burden
  of tracking line breaks in what would otherwise be a chunky stream with no
  assurances.

This class can be used to signal the accumulation of text only if a
  complete "line" is received from upstream.

Call-time semantics of LineEndingCoDec::pushBuffer() are always
  independent of anything being done by this class. That is: a call to
  LineEndingCoDec::pushBuffer() will never be construed as being a "line".
  If a caller intends a line-ending, it should pass buffers that have as their
  last bytes a sequence that will be recognized by this class as a terminator.

However: LineEndingCoDec's treatment of its calls to the efferent->pushBuffer()
  can optionally be made to coincide with received (and possibly transformed)
  termination sequences.

This feature is controlled by two booleans, with their binomial expansion of
  behavioral expectations given below:

                               holdUntilBreak(false)  |  holdUntilBreak(true)
                             +------------------------|------------------------
isometricCallAndBreak(false) |    MODE 0 (default)    |        MODE 1
isometricCallAndBreak(true)  |        MODE 2          |        MODE 2

Behavior of LineEndingCoDec::pushBuffer():
MODE 0: (efferent figures it out if it cares)
  Buffers will be accepted, transformed (if necessary), and forwarded as allowed
  by the downstream capacity on every call, with call-break semantics entirely
  decoupled from buffer content.


MODE 1: (efferent would like to be given several complete lines-at-a-time)
  Buffers will be accepted, transformed (if necessary), and forwarded as allowed
  by the downstream capacity on every call, with call-break semantics concerned
  only with breaking on a final terminal sequence (and perhaps encapsulating many).

Example: Given input buffer offered in a single call to pushBuffer()...
  "This string \n will be call-broken \n only once \n but this will never be taken."

...this output will come through on the next call...
  "This string \n will be call-broken \n only once \n"

...and the remainder of the string " but this will never be taken." will be left
  unclaimed in the offered buffer. And if it is offered again without a line
  terminator having subsequently been added, it will reject the buffer entirely.


MODE 2: (efferent would like to be given a single line-at-a-time)
  Buffers will be accepted, transformed (if necessary), and forwarded as allowed
  by the downstream capacity on every call, with call-break semantics divided to
  coincide directly with terminal sequences. That is, every received "line" will
  be divided out into its own call to efferent->pushBuffer().
  In the example given above, the same content would be delivered across three
  call-breaks, and will leave exactly the same content as was left in MODE 1.



Rules:
--------------------------------------------------------------------------------
1) hold_until_break will only permit passage of the buffer if it contains
    a break, and if so, only forwards the buffer up to (and including) the
    last break in the offered buffer.
2) isometric_call_to_break implies hold_until_break (it is a more-severe form
    of it). If set, the codec will chunk the inbound data by line-breaks,
    and will forward each to the downstream BufferAccepter, one at a time.
3) Replacement is not assumed. With no replacement requested, this class will
    simply chunk output using the specified LineTerms (if any).


*/

#include "../BufferAccepter.h"
#include "../../../TimerTools/TimerTools.h"


#ifndef __C3P_TEXT_LINE_CODEC_H__
#define __C3P_TEXT_LINE_CODEC_H__


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
    MultiStringSearch(const uint8_t TC) :
      _MAX_SEARCH_TERMS(TC), _defs_added(0), _sdef_pool(nullptr), _src(nullptr),
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



/*
*/
class LineEndingCoDec : public BufferCoDec {
  public:
    LineEndingCoDec(BufferAccepter* targ = nullptr, LineTerm t = LineTerm::ZEROBYTE) : BufferCoDec(targ), _term_seq(t) {};
    ~LineEndingCoDec() {};

    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    /* Homogenization feature */
    void replaceOccurrencesOf(LineTerm, bool);
    bool replaceOccurrencesOf(LineTerm);

    /* Operating LineTerm */
    inline void setTerminator(LineTerm x) {          _term_seq = x;           };
    inline LineTerm getTerminator() {                return _term_seq;        };

    /* Chunking feature */
    inline bool holdUntilBreak() {        return (_isometric_call_to_break | _hold_until_break);  };
    inline bool isometriCallAndBreak() {  return (_isometric_call_to_break);  };
    void holdUntilBreak(bool);
    void isometricCallAndBreak(bool);


  private:
    LineTerm        _term_seq;
    uint8_t         _replacement_mask;
    bool            _hold_until_break;
    bool            _isometric_call_to_break;

    int8_t  _push_with_callbreak(StringBuilder*);
};


#endif  // __C3P_TEXT_LINE_CODEC_H__
