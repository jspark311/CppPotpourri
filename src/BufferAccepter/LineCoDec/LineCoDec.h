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


A class to enforce conformity and grouping of line-endings.

This class is the gateway between definitions of what defines a "line" of text
  for internal firmware versus any external system.
This class can be used to signal the accumulation of text only until a
   complete line is received.

Rules:
1) hold_until_break will only permit passage of the buffer if it contains
    a break, and if so, only forwards the buffer up to (and including) the
    last break in the offered buffer.
2) isometric_call_to_break implies hold_until_break (it is a more-severe form
    of it). If set, the codec will chunk the inbound data by line-breaks,
    and will forward each to the downstream BufferAccepter, one at a time.
3) Replacement is not assumed. With no replacement requested, this class will
    simply chunk output using the specified LineTerm.
*/

#include "../BufferAccepter.h"
#include "../../StopWatch.h"


#ifndef __C3P_TEXT_LINE_CODEC_H__
#define __C3P_TEXT_LINE_CODEC_H__

/*
*/
class LineEndingCoDec : public BufferAccepter {
  public:
    LineEndingCoDec(BufferAccepter* targ = nullptr, LineTerm t = LineTerm::ZEROBYTE) : _output_target(targ), _term_seq(t) {};
    ~LineEndingCoDec() {};

    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    inline BufferAccepter* outputTarget() {          return _output_target;   };
    inline void outputTarget(BufferAccepter* x) {    _output_target = x;      };

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
    BufferAccepter* _output_target;
    LineTerm        _term_seq;
    uint16_t        _replacement_mask;
    bool            _hold_until_break;
    bool            _isometric_call_to_break;
};


#endif  // __C3P_TEXT_LINE_CODEC_H__
