/*
File:   C3P_B64.h
Author: J. Ian Lindsay
Date:   2023.07.29

A CoDec for the Base64 transform based on William Sherif's NibbleAndAHalf, which
  I have hard-forked and modified. His original header comment is presenved below.
  https://github.com/superwills/NibbleAndAHalf/
I have reproduced his original commentary, where it still applies. And may have
  changed it to reflect my other changes or operation of my wrappers.
                                                               ---J. Ian Lindsay
*/

/*
  https://github.com/superwills/NibbleAndAHalf -- Fast base64 encoding and decoding.
  version 1.0.1, Feb 1, 2022 812a

  Copyright (C) 2013 William Sherif

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  William Sherif

  YWxsIHlvdXIgYmFzZSBhcmUgYmVsb25nIHRvIHVz
*/

#ifndef __C3P_CODEC_NIBBLEANDAHALF_H__
#define __C3P_CODEC_NIBBLEANDAHALF_H__

#include "../BufferAccepter.h"

// The COMPILE-TIME SETTING CONFIG_NIBBLEANDAHALF_SAFEBASE64 is really important.
// You need to decide if PARANOIA is more important to you than speed.
//
// Supply this def to disable checks for the validity of base64 ascii strings
// before unbase64'ing that string.  If you #define CONFIG_NIBBLEANDAHALF_UNSAFE_DECODE,
// then the program assumes that all characters in the string sent to unbase64()
// are in the base64 alphabet.  As such if a character is NOT in the base64 alphabet
// your data will be wrong (it will be turned to 0 (as if it were just a base64 'A')).
// Removing this test greatly speeds up unbase64'ing (by about 3-4x).
//#define CONFIG_NIBBLEANDAHALF_UNSAFE_DECODE

/*
* NOTE: C3P opts to eat the speed penalty, since this class is likely to be used
*   in circumstances where the buffer is of foreign construction.
* NibbleAndAHalf's original flag behavior was modified to have the same default
*   outcome (safe decoding) without preprocessor intervention. Intervention will
*   be required to make decoding unsafe, and reap the speed gains.
* The commentary above and throughout my fork was modified to reflect this.
*                                                              ---J. Ian Lindsay
*/

/*******************************************************************************
* Half-duplex BufferAccepters on need one of these wwo separate objects.
* A bi-directional transport will probably want one of each.
*******************************************************************************/

/*
* Encoder
*/
class Base64Encoder : public BufferCoDec {
  public:
    Base64Encoder(BufferAccepter* eff = nullptr);
    ~Base64Encoder() {};

    /* Implementation of BufferAccepter. */
    // NOTE: No local buffering is done. Calls to pushBuffer() are construed as
    //   discrete encoding requests if inputSize() is zero..
    int8_t  pushBuffer(StringBuilder*);
    // NOTE: Pending resolutions to the scaling question, this function will scale.
    int32_t bufferAvailable();

    inline int32_t inputLength() {           return _input_length;  };
    inline void    inputLength(int32_t x) {  _input_length = x;     };


  private:
    int32_t _input_length;    // Used to operate as the chunker. 0 implies no chunking.
    // Converts any binary data to base64 characters.
    // Length of the resultant string is stored in flen
    // (you must pass pointer flen).
    int8_t _encode(StringBuilder* src, StringBuilder* dest);
};



/*
* Decoder
*/
class Base64Decoder : public BufferCoDec {
  public:
    Base64Decoder(BufferAccepter* eff = nullptr) : BufferCoDec(eff) {};
    ~Base64Decoder() {};

    /* Implementation of BufferAccepter. */
    // NOTE: No local buffering is done. Calls to pushBuffer() are construed as
    //   discrete decoding requests if inputSize() is zero.
    int8_t  pushBuffer(StringBuilder*);
    // NOTE: Pending resolutions to the scaling question, this function will scale.
    int32_t bufferAvailable();

    inline int32_t inputLength() {           return _input_length;  };
    inline void    inputLength(int32_t x) {  _input_length = x;     };


  private:
    int32_t _input_length;    // Used to operate as the chunker. 0 implies no chunking.
    // Convert your base64 string haJIh/+ back to binary data.
    // len is the string length and should NOT include the null terminator.
    // Final size will be stored in flen
    // (you must pass pointer flen).
    int8_t _decode(StringBuilder* src, StringBuilder* dest);

    int8_t _b64_str_integrity(const uint8_t*, int);
};

#endif // __C3P_CODEC_NIBBLEANDAHALF_H__
