/*
File:   CoDec.h
Author: J. Ian Lindsay
Date:   2023.07.29

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


An abstract interface for CoDecs.
*/

#include "../StringBuilder.h"
#include "../EnumeratedTypeCodes.h"
#include "../CppPotpourri.h"

/*
* A half-duplex interface built on BufferAccepter.
*/
class C3PCoDec : public BufferAccepter {
  public:
    /* Implementation of BufferAccepter. This is how we accept input. */
    int8_t provideBuffer(StringBuilder*);
    int32_t bufferAvailable();

    inline void setEfferant(BufferAccepter* cb) {   _efferant = cb;  };


  protected:
    C3PCoDec(BufferAccepter* target) : _efferant(target) {};
    C3PCoDec() : C3PCoDec(nullptr) {};
    ~C3PCoDec() {};

    BufferAccepter* _efferant;
};




/*
* A class to enforce conformity of line-endings via simple search-and-replace.
*
* NOTE: This class is the gateway between definitions of what defines a "line"
*   of text for internal firmware versus any external system.
*
* NOTE: This class can be used to signal the accumulation of text only until a
*   complete line is received.
*/
class LineEndingCoDec : public BufferAccepter {
  public:
    LineEndingCoDec(BufferAccepter* targ = nullptr, LineTerm t = LineTerm::ZEROBYTE) : _output_target(targ), _term_seq(t) {};
    ~LineEndingCoDec() {};

    /* Implementation of BufferAccepter. */
    int8_t  provideBuffer(StringBuilder*);
    int32_t bufferAvailable();

    inline BufferAccepter* outputTarget() {          return _output_target;   };
    inline void outputTarget(BufferAccepter* x) {    _output_target = x;      };

    // Input data will be searched for all line-endings that do NOT match this
    //   specified value, and replace them when found.
    inline void setTerminator(LineTerm x) {  _term_seq = x;      };
    inline LineTerm getTerminator() {        return _term_seq;   };


  private:
    BufferAccepter* _output_target;
    LineTerm _term_seq;
};
