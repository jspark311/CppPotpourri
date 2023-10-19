/*
File:   C3P_B64.cpp
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

#include "C3P_B64.h"

// TODO: These are wrong. Calulate the real values once it works.
#define BASE64_DECODE_SCALING_FACTOR 0.8
#define BASE64_ENCODE_SCALING_FACTOR 1.2


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/
// b64 maps 0=>A, 1=>B..63=>/ etc
//                      ----------1---------2---------3---------4---------5---------6---
//                      0123456789012345678901234567890123456789012345678901234567890123
const static char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// unb64 maps A=>0, B=>1.. and all others not present in base64 alphabet to 0.
// You can clearly see here why base64 encoding is a really bloated representation
// of the original data: look how many entries are unused. Each ascii character
// can index any value between 0-255 in an array, but we're only using 64 of
// the available slots for meaningful values, leaving 192/256 values unused.
const static uint8_t unb64[256] = {
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //10
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //20
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //30
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //40
  0,   0,   0,  62,   0,   0,   0,  63,  52,  53, //50
 54,  55,  56,  57,  58,  59,  60,  61,   0,   0, //60
  0,   0,   0,   0,   0,   0,   1,   2,   3,   4, //70
  5,   6,   7,   8,   9,  10,  11,  12,  13,  14, //80
 15,  16,  17,  18,  19,  20,  21,  22,  23,  24, //90
 25,   0,   0,   0,   0,   0,   0,  26,  27,  28, //100
 29,  30,  31,  32,  33,  34,  35,  36,  37,  38, //110
 39,  40,  41,  42,  43,  44,  45,  46,  47,  48, //120
 49,  50,  51,   0,   0,   0,   0,   0,   0,   0, //130
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //140
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //150
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //160
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //170
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //180
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //190
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //200
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //210
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //220
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //230
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //240
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //250
  0,   0,   0,   0,   0,   0,
};


// Was previosuly a macro.
// #define isMultipleOf(a,x) (!((a)%x))
static bool b64_is_multiple_of(int a, int x) { return (0 == (a % x));  }


// Was previosuly a macro.
// #define isbase64ValidChr( ch )
// Checks the integrity of a base64 string to make sure it is
// made up of only characters in the base64 alphabet (array b64)
// = is NOT considered a valid base64 chr, it's only valid at the end for padding
static bool b64_is_valid_character(const uint8_t VAL) {
  // TODO: I think this can be made much faster
  //   with a lookup table / switch. ---J. Ian Lindsay
  bool ret = ('0' <= VAL && VAL <= '9');
  if (!ret) {   ret |= ('A' <= VAL && VAL <= 'Z');   }
  if (!ret) {   ret |= ('a' <= VAL && VAL <= 'z');   }
  if (!ret) {   ret |= ('+' == VAL);                 }
  if (!ret) {   ret |= ('/' == VAL);                 }
  return ret;
}



/*******************************************************************************
* Base64Encoder
*******************************************************************************/
Base64Encoder::Base64Encoder(BufferAccepter* eff) :
  BufferCoDec(eff), _input_length(0) {};

int8_t Base64Encoder::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    const int32_t PUSH_LEN      = ((nullptr != buf) ? buf->length() : 0);
    const int32_t AVAILABLE_LEN = bufferAvailable();
    const int32_t TAKE_LEN      = ((0 == _input_length) ? PUSH_LEN : _input_length);
    if ((TAKE_LEN > 0) && (TAKE_LEN <= PUSH_LEN)) {
      // If the pushed length is more than 0, and at least the size we need, make
      //   sure we'll have space for the result.
      if (TAKE_LEN < AVAILABLE_LEN) {
        // Take the bytes, and transform them.
        StringBuilder tmp_src;
        StringBuilder tmp_dest;
        tmp_src.concatHandoffLimit(buf, TAKE_LEN);
        if (0 == _encode(&tmp_src, &tmp_dest)) {
          _efferant->pushBuffer(&tmp_dest);
          ret = (PUSH_LEN == TAKE_LEN) ? 1 : 0;
        }
        else {
          buf->prependHandoff(&tmp_src);
        }
      }
    }
  }
  return ret;
}


int32_t Base64Encoder::bufferAvailable() {
  int32_t ret = 0;
  if (nullptr != _efferant) {
    ret = _efferant->bufferAvailable() * BASE64_ENCODE_SCALING_FACTOR;
  }
  return ret;
}


// Converts any binary data to base64 characters.
// (you must pass pointer flen). flen is strlen, it doesn't
// include the null terminator.
// A common trip-up is not passing your null terminator into
// this function, if you want your C string to be fully encoded,
// you have to pass strlen(str)+1 to as binaryData as I have in the
// examples.
int8_t Base64Encoder::_encode(StringBuilder* src, StringBuilder* dest) {
  int8_t ret = -1;
  // I look at your data like the stream of unsigned bytes that it is
  const uint8_t* src_buf = (const uint8_t*) src->string();
  const int32_t  src_len = src->length();

  // PAD. Base64 is all about breaking the input into SEXTETS, or 6-bit inputs.
  // If you have 1 byte of input, that's 8 bits, not 6. So it won't fit.
  // We need the input to be a multiple of 6. So 8 bits would be padded
  // by 2 bytes to make the total input size 24 bits, which is divisible by 6.
  // A 2 byte input is 16 bits, which is not divisible by 6. So we pad it
  // by 1 byte to make it 24 bits, which is now divisible by 6.
  // We use modulus 3 bytes below because that's 24 bits, and 24 bits is
  // the lowest number that is both divisible by 6 and 8. We need the final
  // output data is to both be divisible by 6 and 8.
  int lenMod3 = src_len % 3;
  int pad = ((lenMod3&1)<<1) + ((lenMod3&2)>>1); // 2 gives 1 and 1 gives 2, but 0 gives 0.

  int flen = (4*(src_len + pad))/3 ; // (len+pad) IS divisible by 3
  // So, final length IS a multiple of 4 for a valid base64 string.

  // Allocate enough space for the base64 string result.
  // and one for the null, which is NOT counted in flen.
  char* base64String = (char*) malloc(flen + 1);

  if(nullptr != base64String) {
    // EXTRACTING SEXTETS FROM THE OCTETS.
    //     byte0       byte1       byte2
    // +-----------+-----------+-----------+
    // | 0000 0011   0111 1011   1010 1101 |
    // +-AAAA AABB   BBBB CCCC   CCDD DDDD
    //
    // In 3 bytes (really, 3 "octets") there are __4__ sextets.
    // You can see that from the diagram above. byte0 (0000 0011) contains
    // the first sextet (AAAA AA) and 2 bits of the 2nd sextet (BB).
    // byte1 contains the next 4 bits of the 2nd sextet (BBBB) and 4 bits
    // of the 3rd sextet (CCCC). byte2 has 2 bits of the 3rd sextet and
    // all 6 bits of the 4th sextet.
    //
    // You can see why we process in groups of 3 bytes: because 3*8 = 24
    // and 24 is the lowest common multiple between 6 and 8. To divide
    // a group of bytes EVENLY into groups of 6, the number of bytes has to
    // be a multiple of 3.

    // Talking in bits, the input already HAS to be a multiple of 8 (because you just
    // can't have anything smaller than a byte saved to memory or disk on modern
    // computers). To successfully convert the bitstream into groups of 6 bits, we'll force
    // the input bitstream to being a MULTIPLE OF 24, so that it will evenly
    // divide by 6.

    // For that reason, we have the concept of PADDING: if the original octet
    // stream is NOT a multiple of 3, then we pad it with 1 or 2 extra bytes
    // so that it is a multiple of 3.

    // So without further ado let's extract the 4 sextets from the 3 octets!
    // Convert sextets in stream into the base64 alphabet using b64 array
    // the value in 6 bits can never be larger than 63, but the b64 array
    // protects us from OOB accesses anyway by providing

    // We want to shift the first 6 bits in the above diagram down to sitting
    // flushed to the right. So we want bin[0] (containing AAAA AABB) to just
    // become 00AA AAAA. We do that with a shift right of 2 bits.

    // We take the number that comes out of that and immediately convert it to
    // the base64 character for that number by doing a direct lookup into the
    // b64 encoding array.

    // We devise 4 formulae below, SEXTET1, SEXTET2, SEXTET3 and SEXTET4. They
    // are used to extract the 4 sextets from the 3 octets that we have.
    #define SEXTET_A(byte0) (byte0 >> 2)
    // Note that no mask needed since BYTE0 is unsigned, so 0's always come in from left
    // (even though there is implicit int promotion on R&L sides prior to actual bitshift).

    // the second sextet BBBBBB is part of the first byte and partly in the 2nd byte.
    //   BYTE0       BYTE1
    // AAAA AABB   BBBB CCCC
    // The first part takes the lower 2 bits of the first byte and pushes them
    // LEFT 4: (AAAA AABB becomes 00BB 0000), then bitwise ORs to it the top 4 bits of
    // BYTE1, shifted RIGHT 4 (BBBB CCCC becomes 0000 BBBB).
    #define SEXTET_B(byte0, byte1) (((0x3&byte0) << 4) | (byte1 >> 4))

    // 3rd sextet CCCCCC is lower nibble of 2nd byte and upper half nibble of 3rd byte.
    //   BYTE1       BYTE2
    // BBBB CCCC   CCDD DDDD
    // From BYTE1, we need to get rid of the BBBB in the front, so we mask
    // those off with 0xf (0000 1111). Then we shift BYTE1 LEFT 2
    // (BBBB CCCC becomes 00CC CC00).
    // We need to fill in the bottom 2 bits of 00CC CC00 with the top 2 bits
    // in BYTE2. So we just shift BYTE2 right by 6 bits (CCDD DDDD becomes 0000 00CC).
    #define SEXTET_C(byte1, byte2) (((0xf&byte1) << 2) | (byte2 >> 6))

    // 4th sextet
    // already low order, just mask off 2 hiorder bits
    //   BYTE2
    // CCDD DDDD
    // We just want to mask off the top 2 bits, use mask 0011 1111 or just 0x3f
    #define SEXTET_D(byte2) (0x3f&byte2)

    int i = 0, byteNo; // result counter, and which byte we're on of the original source data.
    // I still need these variables after the loop
    for( byteNo = 0 ; byteNo <= (src_len-3) ; // This loop is NOT entered for if there
      // are trailing bytes that are not a multiple of 3 bytes,
      // since we skip in 3's.
      // If there WAS padding, skip the last 3 octets and process below.
      // 0=>no, 1=>no, 2=>no, 3=>ONCE,4=>ONCE,5=>ONCE, 6=>2x..
      byteNo+=3 ) // jump in 3's
    {
      // Use uint8_t so shifts left will always bring in 0's
      uint8_t BYTE0 = src_buf[byteNo];
      uint8_t BYTE1 = src_buf[byteNo+1];
      uint8_t BYTE2 = src_buf[byteNo+2];

      // To form the base64String, we make lookups with the base64 numeric
      // values into the base64 "alphabet" that is present in the b64 array.
      base64String[i++] = b64[ SEXTET_A(BYTE0) ];
      base64String[i++] = b64[ SEXTET_B(BYTE0, BYTE1) ];
      base64String[i++] = b64[ SEXTET_C(BYTE1, BYTE2) ];
      base64String[i++] = b64[ SEXTET_D(BYTE2) ];
    }

    // The last 3 octets must be converted carefully as if len%3==1 or len%3==2 we must
    // "pretend" there are additional bits at the end.
    if (1 == pad) {
      uint8_t BYTE0 = src_buf[byteNo];
      uint8_t BYTE1 = src_buf[byteNo+1];
      // When len%3==2 (2,5,8,11) (missing 1 byte).
      //   - 3 sextets (C is 0 padded)
      //    bin[0]       bin[1]      bin[2]
      // +-----------+-----------+-----------+
      // | 0000 0011   1111 1111   ~~~~ ~~~~ |
      // +-AAAA AABB   BBBB CCCC   XXXX XXXX
      // Here all the ~ are actually going to be considered __0__'s.
      base64String[i++] = b64[ SEXTET_A(BYTE0) ] ;
      base64String[i++] = b64[ SEXTET_B(BYTE0, BYTE1) ] ;

      // We can't use the SEXTET3 formula because we only have 2 bytes to work
      // with. The 3rd byte (BYTE2) is actually 0 here. You could call
      // SEXTET3(BYTE1, 0), but to save some ops we just write what will actually
      // be needed here only.
      base64String[i++] = b64[ (0xf&BYTE1) << 2 ] ;

      // Last one is = to indicate there has been a padding of 1 byte.
      base64String[i++] = '=';
    }
    else if (2 == pad) { //len%3==1 (1,4,7,10)
      uint8_t BYTE0 = src_buf[byteNo];
      // We are missing 2 bytes. So
      //   - we will only extract 2 sextets when len%3==1
      //   - The 2nd sextet's 2 HI ORDER BITS, NOT LO-ORDER.
      //   - are being specified by the lowest 2 bits of the 1st octet. these should be 0.
      //    bin[0]       bin[1]      bin[2]
      // +-----------+-----------+-----------+
      // | 0000 0011   ~~~~ ~~~~   ~~~~ ~~~~ |
      // +-AAAA AABB   XXXX XXXX   XXXX XXXX
      base64String[i++] = b64[ SEXTET_A(BYTE0) ] ;
      base64String[i++] = b64[ (0x3&BYTE0) << 4 ] ; // "padded" by 0's, these 2 bits are still HI ORDER BITS.
      // Last 2 are ==, to indicate there's been a 2 byte-pad
      base64String[i++] = '=';
      base64String[i++] = '=';
    }
    base64String[i] = 0; // NULL TERMINATOR! ;)
    dest->concatHandoff((uint8_t*) base64String, i);
    ret = 0;  // It is no longer possible to fail.
  }

  return ret;
}


/*******************************************************************************
* Base64Decoder
*******************************************************************************/

int8_t Base64Decoder::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    const int32_t PUSH_LEN      = ((nullptr != buf) ? buf->length() : 0);
    const int32_t AVAILABLE_LEN = bufferAvailable();
    const int32_t TAKE_LEN      = ((0 == _input_length) ? PUSH_LEN : _input_length);
    if ((TAKE_LEN > 0) && (TAKE_LEN <= PUSH_LEN)) {
      // If the pushed length is more than 0, and at least the size we need, make
      //   sure we'll have space for the result.
      if (TAKE_LEN < AVAILABLE_LEN) {
        // Take the bytes, and transform them.
        StringBuilder tmp_src;
        StringBuilder tmp_dest;
        tmp_src.concatHandoffLimit(buf, TAKE_LEN);
        if (0 == _decode(&tmp_src, &tmp_dest)) {
          _efferant->pushBuffer(&tmp_dest);
          ret = (PUSH_LEN == TAKE_LEN) ? 1 : 0;
        }
        else {
          buf->prependHandoff(&tmp_src);
        }
      }
    }
  }
  return ret;
}


int32_t Base64Decoder::bufferAvailable() {
  int32_t ret = 0;
  if (nullptr != _efferant) {
    ret = _efferant->bufferAvailable() * BASE64_DECODE_SCALING_FACTOR;
  }
  return ret;
}




int8_t Base64Decoder::_decode(StringBuilder* src, StringBuilder* dest) {
  int8_t ret = -1;
  const uint8_t* src_buf = (const uint8_t*) src->string();
  const int32_t  src_len = src->length();

  #ifndef CONFIG_NIBBLEANDAHALF_UNSAFE_DECODE
    if (0 != _b64_str_integrity(src_buf, src_len))  return -2;
  #endif

  int pad = 0;
  if(src_len > 1) {
    // Count == on the end to determine how much it was padded.
    if('=' == src_buf[src_len - 1]) { ++pad; }
    if('=' == src_buf[src_len - 2]) { ++pad; }
  }

  // You take the ascii string len and divide it by 4
  // to get the number of 3 octet groups. You then *3 to
  // get #octets total.
  // If len<4, we makes sure you get a flen of 0, because that's not even
  // a valid base64 string at all.
  int flen = 3*(src_len/4) - pad;
  if (flen < 0) {
    flen = 0;
  }
  uint8_t* bin = (uint8_t*) malloc(flen);
  if(nullptr == bin) {
    return -3;
  }

  int cb = 0; // counter for bin
  int charNo; // counter for what base64 char we're currently decoding

  // NEVER do the last group of 4 characters if either of the
  // last 2 chars were pad.
  for (charNo = 0; charNo <= src_len - 4 - pad; charNo += 4) {
    // Get the numbers each character represents
    // Since ascii is ONE BYTE, the worst that can happen is
    // you get a bunch of 0's back (if the base64 string contained
    // characters not in the base64 alphabet).
    // The only way unbase64 will TELL you about this though
    // is if you #define SAFEBASE64 (particularly because
    // there is a 3-4x performance hit, just for the integrity check.)
    int A=unb64[src_buf[charNo]];
    //printf( "[%4d] %c => %d\n", charNo, ascii[charNo], A ) ;
    int B=unb64[src_buf[charNo+1]];
    //printf( "[%4d] %c => %d\n", charNo+1, ascii[charNo+1], B ) ;
    int C=unb64[src_buf[charNo+2]];
    //printf( "[%4d] %c => %d\n", charNo+2, ascii[charNo+2], C ) ;
    int D=unb64[src_buf[charNo+3]];
    //printf( "[%4d] %c => %d\n", charNo+3, ascii[charNo+3], D ) ;

    // Just unmap each sextet to THE NUMBER it represents.
    // You then have to pack it in bin,
    // we go in groups of 4 sextets,
    // and pull out 3 octets per quad of sextets.
    //    bin[0]       bin[1]      bin[2]
    // +-----------+-----------+-----------+
    // | 0000 0011   0111 1011   1010 1101 |
    // +-AAAA AABB   BBBB CCCC   CCDD DDDD
    // or them
    bin[cb++] = (A<<2) | (B>>4) ; // OR in last 2 bits of B

    // The 2nd byte is the bottom 4 bits of B for the upper nibble,
    // and the top 4 bits of C for the lower nibble.
    bin[cb++] = (B<<4) | (C>>2) ;
    bin[cb++] = (C<<6) | (D) ; // shove C up to top 2 bits, or with D
  }

  // If the length of the string were not a multiple of 4, then the string
  // was damaged and some data was lost.
  if (b64_is_multiple_of(src_len, 4)) {
    if (1 == pad) {
      // 1 padding character.
      //    bin[0]       bin[1]      bin[2]
      // +-----------+-----------+-----------+
      // | 0000 0011   1111 1111   ~~~~ ~~~~ |
      // +-AAAA AABB   BBBB CCCC   XXXX XXXX
      // We can pull 2 bytes out, not 3.
      // We have __3__ characters A,B and C, not 4.
      int A=unb64[src_buf[charNo]];
      int B=unb64[src_buf[charNo+1]];
      int C=unb64[src_buf[charNo+2]];

      bin[cb++] = (A<<2) | (B>>4) ;
      bin[cb++] = (B<<4) | (C>>2) ;
    }
    else if (2 == pad) {
      //    bin[0]       bin[1]      bin[2]
      // +-----------+-----------+-----------+
      // | 0000 0011   ~~~~ ~~~~   ~~~~ ~~~~ |
      // +-AAAA AABB   XXXX XXXX   XXXX XXXX
      int A=unb64[src_buf[charNo]];
      int B=unb64[src_buf[charNo+1]];
      bin[cb++] = (A<<2) | (B>>4) ;
    }
  }

  ret = 0;
  dest->concatHandoff(bin, cb);
  return ret;
}



// Tells you if a string is valid base64, which means its length is
// a multiple of 4, and it contains only valid base64 chrs.
// There are some invalid unbase64 strings, even when they are comprised
// of completely valid characters. An example is "==". That's a 0-length
// piece of data that says it is padded by 2 bytes at the end. Well, you
// only need to pad by 2 bytes if the number of bits in the original data
// was not evenly divisible by 6. 0%6==0, so something's clearly wrong here.
/**
* @return 0 on success
*        -1 on bad length
*        -1 on bad character
*        -2 on bad padding
*        -3 on bad padding
*        -3 on bad ending
*/
int8_t Base64Decoder::_b64_str_integrity(const uint8_t* ascii, int len) {
  int8_t ret = 0;
  // The base64 string is somewhat inflated, since each ASCII character
  // represents only a 6-bit value (a sextet). That leaves 2 bits wasted per 8 bits used.
  // More importantly, for the sextet stream you're getting here (inside
  // an octet stream) to be VALID, THE LENGTH HAS TO BE A MULTIPLE OF 4.
  // You can see in the base64 function above, the algorithm always writes
  // into the final base64 string in groups of __4__.

  // So from there, you can see a valid base64 string has just gotta have a
  // length that is a multiple of 4.

  // If it does not, then it simply isn't valid base64 and the string should
  // be rejected. There really is little sense in trying to decode invalid
  // base64, because it's probably some kind of attack.

  // If the length is not a multiple of 4, it's invalid base64.
  // Here, the empty string will be valid base64 because it represents empty data.
  if( len % 4 )  return -1;

  // LOOKING FOR BAD CHARACTERS
  int i;
  for(i = 0; i < len - 2; i++) {
    if( !b64_is_valid_character(ascii[i]) ) {
      //printf( "ERROR in base64integrity at chr %d [%c]. String is NOT valid base64.\n", i, ascii[i] ) ;
      return -1;
    }
  }

  // Only last 2 can be '='
  // Check 2nd last:
  if( ascii[i]=='=' )
  {
    // If the 2nd last is = the last MUST be = too
    if( ascii[i+1] != '=' )
    {
      //printf( "ERROR in base64integrity at chr %d.\n"
      //"If the 2nd last chr is '=' then the last chr must be '=' too.\n "
      //"String is NOT valid base64.", i ) ;
      return -2;
    }
  }
  else if (!b64_is_valid_character(ascii[i])) {  // not = or valid base64
    // 2nd last was invalid and not '='
    //printf( "ERROR in base64integrity at chr %d (2nd last chr). String is NOT valid base64.\n", i ) ;
    return -3;
  }

  // check last

  i++;
  if(('=' != ascii[i]) && !b64_is_valid_character(ascii[i])) {
    printf( "ERROR in base64integrity at chr %d (last chr). String is NOT valid base64.\n", i);
    return -4;
  }

  return ret;
}
