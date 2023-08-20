/*
File:   CoDec.h
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

#include "../CppPotpourri.h"


// TODO: Check that we aren't doing replacement at the trailing edge if there is
//   a chance that the search will miss a multi-byte sequence. Might-shouldn't
//   use StringBuilder::replace()...
//   Can't easily leverage the tokenizer, either, since some termination
//   sequences are multi-byte, and multiple empty lines will be crushed into a
//   single line-break.
int8_t LineEndingCoDec::provideBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  if (nullptr != _output_target) {
    // TODO: There may be a smarter way to do this with less branching, and
    //   without calling StringBuilder::replace().
    switch (_term_seq) {
      case LineTerm::ZEROBYTE:  // "\0"
        // TODO: This might be special... Perverrse consequence warning.
        //   Write the unit tests for this before trying to implement it.
        break;
      case LineTerm::CR:
        buf->replace("\r\n", "\r");  // Replace the complex case first.
        buf->replace("\n",   "\r");
        ret = _output_target->provideBuffer(buf);
        break;
      case LineTerm::LF:
        buf->replace("\r\n", "\n");  // Replace the complex case first.
        buf->replace("\r",   "\n");
        ret = _output_target->provideBuffer(buf);
        break;
      case LineTerm::CRLF:
        // TODO: This won't work because the end result will be things like "\r\r\n".
        //   More reason to not try to use StringBuilder::replace()...
        //buf->replace("\r",   "\r\n");
        //buf->replace("\n",   "\r\n");
        ret = _output_target->provideBuffer(buf);
        break;
    }
  }
  return ret;
}


// NOTE: This function will over-report if doing a conversion that
//   increases the byte count versus the input, and it will under-report if
//   conversion decreases the count. This is perfectly acceptable behavior if
//   the results of return values are observed within contractual limits.
int32_t LineEndingCoDec::bufferAvailable() {
  return ((nullptr != _output_target) ? _output_target->bufferAvailable() : 0);
}
