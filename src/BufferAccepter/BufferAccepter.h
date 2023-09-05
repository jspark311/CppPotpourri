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
* Interfaces for moving buffers in a predictable fashion. Also, some optional
*   helper classes built on that capability.
*
* NOTE: This idea was the fundamental idea behind Manuvr's BufferPipe class,
*   which was not pure virtual, carried far more implementation burden, and led
*   to all manner of inheritance-fueled maintenance nightmares. Please carefully
*   consider the contracts in the README.md file before extending (or
*   espescially changing) these interfaces. Many things depend on the contract.
*******************************************************************************/

/*
* An interface class for accepting a buffer.
*
* A class would implement BufferAccepter to expose a means of accepting a
*   formless buffer from a source that doesn't need to know the specifics of
*   what is to be done with the buffer, nor how.
*/
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


/*
* A half-duplex interface built on BufferAccepter.
*
* TODO: The might be bikeshedding, but having the extra member in a controled
*   namespace might be useful. We'll see... Expect flux.
*   If this idea is retained, it should received a formalized contract and full
*   unit-testing thereof, as BufferAccepter has.
*
* The basic intent is, we need a generallized class that is not only itself a
*   BufferAccepter, but is also expected to produce buffers as a result of
*   afferant pushes. That is, it is neither a sink, nor a source.
*   Or it is both. But not only one or the other.
*
* TODO: Well.... Is it neither or both? It's shaping up to be both, as long as
*   it is degrading types down to BufferAccepter... But we haven't a need of
*   defining the shape of a source, explicitly.
*
* NOTE: This class degrades types of potentially other CoDocs. That is, it only
*   deals with contracts on BufferAccepter's terms. Not its own, or those of its
*   children.
*
* TODO: Should bufferAvailable() make an attempt to do its own scaling to report
*   to callers? IE, if a CoDec knows that it will change the size of a buffer,
*   should it report the honest value from downstream, or scale it for its own
*   reply? The BufferAccepter contract says that either are acceptable here.
* An argument for "always honest" would be that some CoDecs (compression) will
*   have absolutely no idea what their scaler would be until they are given
*   specific data (which they can't be). Maybe report honest in such cases?
* The argument for "always scaled" is that upstream callers will be unable to
*   make reliable choices in their length-dependent behaviors without having
*   unobtainable knowledge about their downstream ratios.
* Make a seperate call-chain for this? Can't do it with type degredataion.
* Make a parameter to bufferAvailable()? That could work, but might be ugly.
*/
class BufferCoDec : public BufferAccepter {
  public:
    /* Extention carries no implementation of BufferAccepter. */
    //virtual int8_t  pushBuffer(StringBuilder*) =0;
    //virtual int32_t bufferAvailable() =0;

    inline void setEfferant(BufferAccepter* x) {   _efferant = x;  };
    // TODO: Might be better to do this, so a chance can be made to accept or reject
    //   potential efferants based on class criteria. Or reset child-scoped variables
    //   when the efferant changes. Keeping it simple for now...
    //virtual int8_   setEfferant(BufferAccepter* x) =0;


  protected:
    BufferCoDec(BufferAccepter* target = nullptr) : _efferant(target) {};
    ~BufferCoDec() {};

    BufferAccepter* _efferant;
};


/*
* A full-duplex interface built on BufferAccepter.
*
* TODO: It would be nice to have this eventually. But doing it in a way that is
*   composable (and safe (and not confusing (and not a nightmare to maintain)))
*   is a real challenge. Might be best to make it parallel to (and not depend
*   upon) BufferAccepter.
*/


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
};

// TODO: Write proper CoDec transform objects: CBOR, Delta97.

#endif  // __C3P_BUFFERACCEPTER_H__
