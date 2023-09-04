/*
File:   BufferAccepter.h
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


An abstract interface for buffers.
*/

#include "../StringBuilder.h"
#include "../EnumeratedTypeCodes.h"
#include "../CppPotpourri.h"

#ifndef __C3P_BUFFERACCEPTER_H__
#define __C3P_BUFFERACCEPTER_H__


/*******************************************************************************
* Interface for moving buffers in a predictable fashion. Also, some optional
*   helper classes built on that capability.
*
* A class would implement BufferAccepter to expose a means of accepting a
*   formless buffer from a source that doesn't need to know the specifics of
*   what is to be done with the buffer, nor how.
* NOTE: This idea was the fundamental idea behind Manuvr's BufferPipe class,
*   which was not pure virtual, and carried far more implementation burden.
*******************************************************************************/

/* An interface class for accepting a buffer. */
class BufferAccepter {
  public:
    /**
    * Provides a heap-based buffer with fully-realized ownership management.
    *
    * @param is the pointer to the managed container for the content.
    * @return -1 to reject buffer, 0 to accept with partial claim, 1 to accept with full claim.
    */
    virtual int8_t pushBuffer(StringBuilder*) =0;

    /**
    * @return the number of bytes available in the next stage of buffering.
    */
    virtual int32_t bufferAvailable() =0;
};


/*******************************************************************************
* Helpers and utility classes surrounding BufferAccepter.
*******************************************************************************/

/* A trivial class to collect buffers into a StringBuilder. */
class StringBuilderSink : public StringBuilder, public BufferAccepter {
  public:
    StringBuilderSink(const int32_t MAX_L) : MAX_CAPTURE_LENGTH(MAX_L) {};
    ~StringBuilderSink() {};

    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder* buf);
    int32_t bufferAvailable();

  private:
    const int32_t MAX_CAPTURE_LENGTH;
};


/* A class to fork a string in a safe way. */
class BufferAccepterFork : public BufferAccepter {
  public:
    BufferAccepterFork(BufferAccepter* lh = nullptr, BufferAccepter* rh = nullptr) :
      _left_hand(lh), _right_hand(rh),
      _left_drift(0), _right_drift(0) {};
    ~BufferAccepterFork() {};

    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    inline BufferAccepter* leftHand() {          return _left_hand;   };
    inline BufferAccepter* rightHand() {         return _right_hand;  };
    inline void leftHand(BufferAccepter* x) {    _left_hand  = x;  _left_drift  = 0;   };
    inline void rightHand(BufferAccepter* x) {   _right_hand = x;  _right_drift = 0;   };

  private:
    BufferAccepter* _left_hand;
    BufferAccepter* _right_hand;
    int32_t _left_drift;
    int32_t _right_drift;

    //int8_t _push_to_hand(BufferAccepter*, StringBuilder*);
};

// TODO: Write proper CoDec transform objects: CBOR, Delta97, Base64.



/*
* A half-duplex interface built on BufferAccepter.
*/
class C3PCoDec : public BufferAccepter {
  public:
    /* Implementation of BufferAccepter. This is how we accept input. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    inline void setEfferant(BufferAccepter* x) {   _efferant = x;  };


  protected:
    C3PCoDec(BufferAccepter* target) : _efferant(target) {};
    C3PCoDec() : C3PCoDec(nullptr) {};
    ~C3PCoDec() {};

    BufferAccepter* _efferant;
};


#endif  // __C3P_BUFFERACCEPTER_H__
