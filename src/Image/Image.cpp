/*
File:   Image.cpp
Author: J. Ian Lindsay
Date:   2019.06.02

*/

/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#if defined(CONFIG_C3P_IMG_SUPPORT)

#include <stdlib.h>
#include <math.h>
#include "Image.h"

/* If character support is enabled, we'll import fonts. */
#include "Fonts/FreeMono12pt7b.h"
#include "Fonts/FreeMono18pt7b.h"
#include "Fonts/FreeMono24pt7b.h"
#include "Fonts/FreeMono9pt7b.h"
#include "Fonts/FreeMonoBold12pt7b.h"
#include "Fonts/FreeMonoBold18pt7b.h"
#include "Fonts/FreeMonoBold24pt7b.h"
#include "Fonts/FreeMonoBold9pt7b.h"
#include "Fonts/FreeMonoBoldOblique12pt7b.h"
#include "Fonts/FreeMonoBoldOblique18pt7b.h"
#include "Fonts/FreeMonoBoldOblique24pt7b.h"
#include "Fonts/FreeMonoBoldOblique9pt7b.h"
#include "Fonts/FreeMonoOblique12pt7b.h"
#include "Fonts/FreeMonoOblique18pt7b.h"
#include "Fonts/FreeMonoOblique24pt7b.h"
#include "Fonts/FreeMonoOblique9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSansBold18pt7b.h"
#include "Fonts/FreeSansBold24pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/FreeSansBoldOblique12pt7b.h"
#include "Fonts/FreeSansBoldOblique18pt7b.h"
#include "Fonts/FreeSansBoldOblique24pt7b.h"
#include "Fonts/FreeSansBoldOblique9pt7b.h"
#include "Fonts/FreeSansOblique12pt7b.h"
#include "Fonts/FreeSansOblique18pt7b.h"
#include "Fonts/FreeSansOblique24pt7b.h"
#include "Fonts/FreeSansOblique9pt7b.h"
#include "Fonts/FreeSerif12pt7b.h"
#include "Fonts/FreeSerif18pt7b.h"
#include "Fonts/FreeSerif24pt7b.h"
#include "Fonts/FreeSerif9pt7b.h"
#include "Fonts/FreeSerifBold12pt7b.h"
#include "Fonts/FreeSerifBold18pt7b.h"
#include "Fonts/FreeSerifBold24pt7b.h"
#include "Fonts/FreeSerifBold9pt7b.h"
#include "Fonts/FreeSerifBoldItalic12pt7b.h"
#include "Fonts/FreeSerifBoldItalic18pt7b.h"
#include "Fonts/FreeSerifBoldItalic24pt7b.h"
#include "Fonts/FreeSerifBoldItalic9pt7b.h"
#include "Fonts/FreeSerifItalic12pt7b.h"
#include "Fonts/FreeSerifItalic18pt7b.h"
#include "Fonts/FreeSerifItalic24pt7b.h"
#include "Fonts/FreeSerifItalic9pt7b.h"
#include "Fonts/Org_01.h"
#include "Fonts/Picopixel.h"
/* End of font import. */

/*******************************************************************************
* Statics and externs
*******************************************************************************/

const char* const Image::formatString(const ImgBufferFormat FMT) {
  switch (FMT) {
    case ImgBufferFormat::UNALLOCATED:     return "UNALLOCATED";
    case ImgBufferFormat::MONOCHROME:      return "MONOCHROME";
    case ImgBufferFormat::GREY_24:         return "GREY_24";
    case ImgBufferFormat::GREY_16:         return "GREY_16";
    case ImgBufferFormat::GREY_8:          return "GREY_8";
    case ImgBufferFormat::GREY_4:          return "GREY_4";
    case ImgBufferFormat::R8_G8_B8_ALPHA:  return "R8_G8_B8_ALPHA";
    case ImgBufferFormat::R8_G8_B8:        return "R8_G8_B8";
    case ImgBufferFormat::R5_G6_B5:        return "R5_G6_B5";
    case ImgBufferFormat::R3_G3_B2:        return "R3_G3_B2";
    default:                               return "UNKNOWN";
  }
}

const float Image::rotationAngle(const ImgOrientation ROT_ENUM) {
  switch (ROT_ENUM) {
    default:
    case ImgOrientation::ROTATION_0:      return 0.0f;
    case ImgOrientation::ROTATION_90:     return 90.0f;
    case ImgOrientation::ROTATION_180:    return 180.0f;
    case ImgOrientation::ROTATION_270:    return 270.0f;
  }
}

/**
* @param ImgBufferFormat
* @return the number of bits to represent a pixel for the given ImgBufferFormat.
*/
const uint8_t Image::_bits_per_pixel(const ImgBufferFormat FMT) {
  switch (FMT) {
    case ImgBufferFormat::MONOCHROME:      return 1;
    case ImgBufferFormat::GREY_4:          return 4;
    case ImgBufferFormat::GREY_8:
    case ImgBufferFormat::R3_G3_B2:        return 8;    // 8-bit color
    case ImgBufferFormat::GREY_16:
    case ImgBufferFormat::R5_G6_B5:        return 16;   // 16-bit color
    case ImgBufferFormat::GREY_24:
    case ImgBufferFormat::R8_G8_B8:        return 24;   // 24-bit color
    case ImgBufferFormat::R8_G8_B8_ALPHA:  return 32;   // 24-bit color with 8-bits of alpha
    case ImgBufferFormat::UNALLOCATED:
    default:  break;
  }
  return 0;
}


/*******************************************************************************
* Helper functions
* TODO: Some of this should probably be re-coded to use the ASM-optimized
*   intrinsics provided by the compiler, if available.
*******************************************************************************/
static inline uint8_t _u8_sat_add(const uint8_t a, const uint8_t b) {
  const uint16_t s = (uint16_t) a + (uint16_t) b;
  return (uint8_t) ((s > 255) ? 255 : s);
}

static inline uint8_t _u8_sat_sub(const uint8_t a, const uint8_t b) {
  return (uint8_t) ((a > b) ? (a - b) : 0);
}

// (a*b)/255 with rounding
static inline uint8_t _u8_mul_255(const uint8_t a, const uint8_t b) {
  const uint16_t p = (uint16_t) a * (uint16_t) b;
  return (uint8_t) ((p + 127) / 255);
}

// Approx perceptual luma, normalized to 0..255 (integer)
// (0.30, 0.59, 0.11)
static inline uint8_t _u8_luma(const uint8_t r, const uint8_t g, const uint8_t b) {
  const uint16_t v = (uint16_t)(r * 30) + (uint16_t)(g * 59) + (uint16_t)(b * 11);
  return (uint8_t) (v / 100);
}

static inline void _unpack_rgba(const uint32_t c, const ImgBufferFormat fmt, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a) {
  uint8_t rr = 0;
  uint8_t gg = 0;
  uint8_t bb = 0;
  uint8_t aa = 0xFF;

  switch (fmt) {
    case ImgBufferFormat::R8_G8_B8_ALPHA:
      rr = (uint8_t) ((c >> 24) & 0xFF);
      gg = (uint8_t) ((c >> 16) & 0xFF);
      bb = (uint8_t) ((c >> 8)  & 0xFF);
      aa = (uint8_t) (c & 0xFF);
      break;

    case ImgBufferFormat::R8_G8_B8:
      rr = (uint8_t) ((c >> 16) & 0xFF);
      gg = (uint8_t) ((c >> 8)  & 0xFF);
      bb = (uint8_t) (c & 0xFF);
      aa = 0xFF;
      break;

    case ImgBufferFormat::R5_G6_B5:
      rr = (uint8_t) (((c >> 11) & 0x1F) << 3);
      gg = (uint8_t) (((c >> 5)  & 0x3F) << 2);
      bb = (uint8_t) ((c & 0x1F) << 3);
      aa = 0xFF;
      break;

    case ImgBufferFormat::R3_G3_B2:
      rr = (uint8_t) (((c >> 5) & 0x07) << 5);
      gg = (uint8_t) (((c >> 2) & 0x07) << 5);
      bb = (uint8_t) ((c & 0x03) << 6);
      aa = 0xFF;
      break;

    case ImgBufferFormat::MONOCHROME:
      if (0 != c) { rr = 0xFF; gg = 0xFF; bb = 0xFF; }
      else {        rr = 0x00; gg = 0x00; bb = 0x00; }
      aa = 0xFF;
      break;

    case ImgBufferFormat::GREY_8:
      rr = (uint8_t) (c & 0xFF);
      gg = rr;
      bb = rr;
      aa = 0xFF;
      break;

    case ImgBufferFormat::GREY_16:
      // Keep the high byte as intensity (common convention)
      rr = (uint8_t) ((c >> 8) & 0xFF);
      gg = rr;
      bb = rr;
      aa = 0xFF;
      break;

    case ImgBufferFormat::GREY_24:
      // Interpret as 24-bit with replicated channels if present; else average.
      {
        const uint8_t r0 = (uint8_t) ((c >> 16) & 0xFF);
        const uint8_t g0 = (uint8_t) ((c >> 8)  & 0xFF);
        const uint8_t b0 = (uint8_t) (c & 0xFF);
        const uint8_t y0 = (uint8_t) (((uint16_t)r0 + (uint16_t)g0 + (uint16_t)b0) / 3);
        rr = y0;
        gg = y0;
        bb = y0;
        aa = 0xFF;
      }
      break;

    default:
      // GREY_4 and UNALLOCATED fall here (unsupported for blending as-is).
      rr = 0;
      gg = 0;
      bb = 0;
      aa = 0;
      break;
  }

  if (r) { *r = rr; }
  if (g) { *g = gg; }
  if (b) { *b = bb; }
  if (a) { *a = aa; }
}


static inline uint32_t _pack_from_rgba(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a, const ImgBufferFormat fmt) {
  uint32_t ret = 0;
  switch (fmt) {
    case ImgBufferFormat::R8_G8_B8_ALPHA:
      ret |= ((uint32_t) r << 24);
      ret |= ((uint32_t) g << 16);
      ret |= ((uint32_t) b << 8);
      ret |= ((uint32_t) a);
      break;

    case ImgBufferFormat::R8_G8_B8:
      ret |= ((uint32_t) r << 16);
      ret |= ((uint32_t) g << 8);
      ret |= ((uint32_t) b);
      break;

    case ImgBufferFormat::R5_G6_B5:
      ret |= ((uint32_t) (r >> 3) << 11);
      ret |= ((uint32_t) (g >> 2) << 5);
      ret |= ((uint32_t) (b >> 3));
      break;

    case ImgBufferFormat::R3_G3_B2:
      ret |= ((uint32_t) (r >> 5) << 5);
      ret |= ((uint32_t) (g >> 5) << 2);
      ret |= ((uint32_t) (b >> 6));
      break;

    case ImgBufferFormat::GREY_8:
      ret = (uint32_t) _u8_luma(r, g, b);
      break;

    case ImgBufferFormat::GREY_16:
      {
        const uint8_t y = _u8_luma(r, g, b);
        ret = ((uint32_t) y << 8) | (uint32_t) y;
      }
      break;

    case ImgBufferFormat::GREY_24:
      {
        const uint8_t y = _u8_luma(r, g, b);
        ret = ((uint32_t) y << 16) | ((uint32_t) y << 8) | (uint32_t) y;
      }
      break;

    case ImgBufferFormat::MONOCHROME:
      ret = (_u8_luma(r, g, b) >= 128) ? 0xFFFFFFFF : 0;
      break;

    default:
      ret = 0;
      break;
  }
  return ret;
}




/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/*
* Constructor
*/
Image::Image(PixUInt x, PixUInt y, ImgBufferFormat fmt, uint8_t* buf) : Image(x, y) {
  _buf_fmt  = fmt;
  _imgflags = 0;
  _buffer   = buf;
}


/*
* Constructor
*/
Image::Image(PixUInt x, PixUInt y, ImgBufferFormat fmt) : Image(x, y) {
  _buf_fmt  = fmt;
}


/*
* Constructor
*/
Image::Image(PixUInt x, PixUInt y) : _x(x), _y(y) {}

/*
* Constructor
*/
Image::Image() : Image(0, 0) {}


/*
* Destructor. Free the buffer if it is our creation.
*/
Image::~Image() {
  if (_is_ours() && allocated()) {
    free(_buffer);
    _buffer = nullptr;
  }
}


/*******************************************************************************
* Buffer management functions
*******************************************************************************/

/**
* Takes the given buffer and assume it as our own.
* Frees any existing buffer. It is the caller's responsibility to ensure that
*   the buffer is adequately sized.
*
* @return true if the buffer replacement succeeded. False if format is unknown.
*/
bool Image::setBuffer(uint8_t* buf) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (_is_ours() && allocated()) {
      free(_buffer);
      _buffer = nullptr;
      _ours(false);
    }
    _buffer = buf;
    return true;
  }
  return false;
}


/**
* Takes the given buffer and format and assume it as our own.
* Frees any existing buffer. It is the caller's responsibility to ensure that
*   the buffer is adequately sized.
*
* @return true always.
*/
bool Image::setBuffer(uint8_t* buf, ImgBufferFormat fmt) {
  if (_is_ours() && allocated()) {
    free(_buffer);
    _buffer = nullptr;
    _ours(false);
  }
  _buf_fmt = fmt;
  _buffer = buf;
  return true;
}


/**
*
*/
bool Image::setBufferByCopy(uint8_t* buf) {
  return setBufferByCopy(buf, _buf_fmt);
}


/**
* TODO: Need to audit memory management here. Unit testing is clearly warranted.
*/
bool Image::setBufferByCopy(uint8_t* buf, ImgBufferFormat fmt) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (fmt == _buf_fmt) {
      if (allocated()) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
      else if (0 == _buffer_allocator()) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
    }
    else {
      ImgBufferFormat old_fmt = _buf_fmt;
      _buf_fmt = fmt;
      if (allocated() && (_bits_per_pixel(old_fmt) == _bits_per_pixel(_buf_fmt))) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
      else if (0 == _buffer_allocator()) {
        memcpy(_buffer, buf, bytesUsed());
        return true;
      }
    }
  }
  else {
    _buf_fmt = fmt;
    if (0 == _buffer_allocator()) {
      memcpy(_buffer, buf, bytesUsed());
      return true;
    }
  }
  return false;
}


bool Image::setSize(PixUInt x, PixUInt y) {
  _x = x;
  _y = y;
  return (0 == _buffer_allocator());
}


/**
*
*/
bool Image::reallocate() {
  return (0 == _buffer_allocator());
}


uint32_t Image::bytesUsed() {
  if (ImgBufferFormat::UNALLOCATED == _buf_fmt) return 0;
  const uint32_t PIXEL_COUNT  = pixels();
  const uint8_t  BPP          = _bits_per_pixel();
  const uint32_t TOTAL_BITS   = (PIXEL_COUNT * BPP);
  const uint32_t PADDING_BYTE = ((TOTAL_BITS & 7) ? 1:0);
  return ((TOTAL_BITS >> 3) + PADDING_BYTE);
};


/*
* Returns 0 on success, -1 on bad class state, -2 on free failure, -3 on malloc fail
*/
int8_t Image::_buffer_allocator() {
  int8_t ret = -1;
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    ret--;
    if (allocated()) {
      if (!_is_ours()) {
        return ret;
      }
      free(_buffer);
    }
    ret--;
    _buffer = (uint8_t*) malloc(bytesUsed());
    if (allocated()) {
      _ours(true);
      memset(_buffer, 0, bytesUsed());
      ret = 0;
    }
  }
  return ret;
}


/**
*
*/
void Image::wipe() {
  if (_is_ours() && allocated()) {
    free(_buffer);
  }
  _x        = 0;
  _y        = 0;
  _buffer   = nullptr;
  _buf_fmt  = ImgBufferFormat::UNALLOCATED;
  _imgflags = 0;
}


/*******************************************************************************
* Serializer functions
*******************************************************************************/

/**
* NOTE: The serializer does not account for orientation.
*/
int8_t Image::serialize(StringBuilder* out) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (allocated()) {
      uint32_t sz = bytesUsed();
      uint8_t buf[11];
      buf[0] = (uint8_t) (_x >> 24) & 0xFF;
      buf[1] = (uint8_t) (_x >> 16) & 0xFF;
      buf[2] = (uint8_t) (_x >> 8) & 0xFF;
      buf[3] = (uint8_t) _x & 0xFF;
      buf[4] = (uint8_t) (_y >> 24) & 0xFF;
      buf[5] = (uint8_t) (_y >> 16) & 0xFF;
      buf[6] = (uint8_t) (_y >> 8) & 0xFF;
      buf[7] = (uint8_t) _y & 0xFF;
      buf[8] = (uint8_t) _buf_fmt;
      buf[9] = (uint8_t) (_imgflags);
      buf[10] = (uint8_t) (_imgflags >> 8);
      out->concat(buf, sizeof(buf));
      out->concat(_buffer, sz);
      return 0;
    }
  }
  return -1;
}


/**
* NOTE: The serializer does not account for orientation.
*/
int8_t Image::serialize(uint8_t* buf, uint32_t* len) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (allocated()) {
      uint32_t sz = bytesUsed();
      *(buf + 0) = (uint8_t) (_x >> 24) & 0xFF;
      *(buf + 1) = (uint8_t) (_x >> 16) & 0xFF;
      *(buf + 2) = (uint8_t) (_x >> 8) & 0xFF;
      *(buf + 3) = (uint8_t) _x & 0xFF;
      *(buf + 4) = (uint8_t) (_y >> 24) & 0xFF;
      *(buf + 5) = (uint8_t) (_y >> 16) & 0xFF;
      *(buf + 6) = (uint8_t) (_y >> 8) & 0xFF;
      *(buf + 7) = (uint8_t) _y & 0xFF;
      *(buf + 8) = (uint8_t) _buf_fmt;
      *(buf + 9) = (uint8_t) _imgflags;
      *(buf + 10) = (uint8_t) (_imgflags >> 8);
      memcpy((buf + 11), _buffer, sz);
      *len = (11+sz);
      return 0;
    }
  }
  return -1;
}


/**
* Serializes the object without the bulk of the data (the image itself). This is
*   for the sake of avoiding memory overhead on an intermediate copy.
* The len parameter will be updated to reflect the actual written bytes. Not
*   including the langth of the full buffer.
*/
int8_t Image::serializeWithoutBuffer(uint8_t* buf, uint32_t* len) {
  if (ImgBufferFormat::UNALLOCATED != _buf_fmt) {
    if (allocated()) {
      *(buf + 0) = (uint8_t) (_x >> 24) & 0xFF;
      *(buf + 1) = (uint8_t) (_x >> 16) & 0xFF;
      *(buf + 2) = (uint8_t) (_x >> 8) & 0xFF;
      *(buf + 3) = (uint8_t) _x & 0xFF;
      *(buf + 4) = (uint8_t) (_y >> 24) & 0xFF;
      *(buf + 5) = (uint8_t) (_y >> 16) & 0xFF;
      *(buf + 6) = (uint8_t) (_y >> 8) & 0xFF;
      *(buf + 7) = (uint8_t) _y & 0xFF;
      *(buf + 8) = (uint8_t) _buf_fmt;
      *(buf + 9) = (uint8_t) _imgflags;
      *(buf + 10) = (uint8_t) (_imgflags >> 8);
      *len = 11;
      return 0;
    }
  }
  return -1;
}


/**
* NOTE: The deserializer does not account for orientation.
*/
int8_t Image::deserialize(uint8_t* buf, uint32_t len) {
  if (ImgBufferFormat::UNALLOCATED == _buf_fmt) {
    // TODO: This is awful, and will absolutely wreck someone later.
    //   Shunt this responsibility to C3PType.
    const uint8_t PIX_AND_FMT_LEN = (sizeof(PixUInt) + 1);
    if (len > PIX_AND_FMT_LEN) {
      #if(9 > CONFIG_C3P_IMG_COORD_BITS)
        PixUInt temp_x = (PixUInt) *(buf + 0);
        PixUInt temp_y = (PixUInt) *(buf + 1);
      #elif(17 > CONFIG_C3P_IMG_COORD_BITS)
        PixUInt temp_x = ((PixUInt) *(buf + 0) << 8) | ((PixUInt) *(buf + 1));
        PixUInt temp_y = ((PixUInt) *(buf + 2) << 8) | ((PixUInt) *(buf + 3));
      #elif(33 > CONFIG_C3P_IMG_COORD_BITS)
        PixUInt temp_x = ((PixUInt) *(buf + 0) << 24) | ((PixUInt) *(buf + 1) << 16) | ((PixUInt) *(buf + 2) << 8) | ((PixUInt) *(buf + 3));
        PixUInt temp_y = ((PixUInt) *(buf + 4) << 24) | ((PixUInt) *(buf + 5) << 16) | ((PixUInt) *(buf + 6) << 8) | ((PixUInt) *(buf + 7));
      #else
        #error Integers for holding pixel addresses cannot be larger than 32-bit.
      #endif
      ImgBufferFormat temp_fmt = (ImgBufferFormat) *(buf + (PIX_AND_FMT_LEN-1));
      uint32_t temp_f = *(buf + PIX_AND_FMT_LEN);
      uint32_t proposed_size = (_bits_per_pixel(temp_fmt) * temp_x * temp_y) >> 3;
      if (proposed_size == (len - (PIX_AND_FMT_LEN+1))) {
        // The derived and declared sizes match.
        _x = temp_x;
        _y = temp_y;
        _buf_fmt  = temp_fmt;
        _imgflags = temp_f;
        return setBufferByCopy(buf + (PIX_AND_FMT_LEN+1)) ? 0 : -2;
      }
    }
  }
  return -1;
}


void Image::printImageInfo(StringBuilder* out, const bool DETAIL) {
  if (DETAIL) {
  }
  else {   // Issue the TL;DR
    out->concatf("(%u x %u) %s\n", _x, _y, formatString(_buf_fmt));
  }
}



/*******************************************************************************
* Color functions
*******************************************************************************/
/*
* NOTE: This is not an inline to avoid notions of platform in the Image.h file.
* If the platform's endianness differs from the desired endiannesss, we flip the
*   byte order of color data before writing it to the buffer.
*/
void Image::bigEndian(bool x) {
  const uint16_t TEST = 0xAA55;
  const bool PF_IS_BIG_ENDIAN = (0xAA == *((uint8_t*) &TEST));
  _img_set_flag(C3P_IMG_FLAG_ENDIAN_FLIP, (PF_IS_BIG_ENDIAN ^ x));
}


/**
*
*/
bool Image::isColor() {
  switch (_buf_fmt) {
    case ImgBufferFormat::R8_G8_B8_ALPHA:    // 32-bit color
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
    case ImgBufferFormat::R3_G3_B2:          // 8-bit color
      return true;
    default:
      break;
  }
  return false;
}


/**
* Converts the color specified in the argument to the best-representation in
*   the buffer's native format.
* Color to grey conversion uses 30/59/11 scalar set with results averaged together.
*/
uint32_t Image::convertColor(uint32_t c, ImgBufferFormat src_fmt) {
  if (src_fmt == _buf_fmt) {
    return c;
  }
  if ((ImgBufferFormat::UNALLOCATED == src_fmt) || (ImgBufferFormat::UNALLOCATED == _buf_fmt)) {
    return 0;
  }
  uint8_t r = 0, g = 0, b = 0, a = 0xFF;
  _unpack_rgba(c, src_fmt, &r, &g, &b, &a);

  // If the source has no alpha channel, treat it as fully opaque.
  if (ImgBufferFormat::R8_G8_B8_ALPHA != src_fmt) {
    a = 0xFF;
  }
  return _pack_from_rgba(r, g, b, a, _buf_fmt);
}


/*******************************************************************************
* Pixel-level operations, and the linkage with the buffer.
*******************************************************************************/

void Image::flipX(bool f) {
  if (_img_flag(C3P_IMG_FLAG_FLIP_X) != f) {
    _img_set_flag(C3P_IMG_FLAG_FLIP_X, f);
    if (nullptr != _buffer) {
      // TODO:
      // If we already have a buffer allocated, apply the transform to the
      //   existing buffer.
    }
  }
}

void Image::flipY(bool f) {
  if (_img_flag(C3P_IMG_FLAG_FLIP_Y) != f) {
    _img_set_flag(C3P_IMG_FLAG_FLIP_Y, f);
    if (nullptr != _buffer) {
      // TODO:
      // If we already have a buffer allocated, apply the transform to the
      //   existing buffer.
    }
  }
}

void Image::orientation(ImgOrientation nu) {
  if (orientation() != nu) {
    _imgflags = (_imgflags & ~C3P_IMG_FLAG_ORIENTATION_MASK) | ((uint16_t) nu);
    if (nullptr != _buffer) {
      // TODO:
      // If we already have a buffer allocated, apply the transform to the
      //   existing buffer.
    }
  }
}


/*
* This function is called internally to remap the given coordinate to a new
*   coordinate that accounts for all of the spatial transforms implied by the
*   settings.
*/
void Image::_remap_for_orientation(PixUInt* xn, PixUInt* yn) {
  switch (orientation()) {
    case ImgOrientation::ROTATION_270:
      strict_swap(xn, yn);
      *yn = (y()-1) - *yn;
      break;
    case ImgOrientation::ROTATION_180:
      *xn = (x()-1) - *xn;
      *yn = (y()-1) - *yn;
      break;
    case ImgOrientation::ROTATION_90:
      strict_swap(xn, yn);
      //*xn = (x()-1) - *xn;
      break;
    case ImgOrientation::ROTATION_0:   // Native format.
      break;
  }

  if (flipX()) {    *xn = (x() - 1) - *xn;  }
  if (flipY()) {    *yn = (y() - 1) - *yn;  }
}


/**
* Takes a color in 32-bit. Squeezes it into the buffer's format, discarding low
*   bits as appropriate. Uses Porter-Duff alpha compositing.
*/
bool Image::setPixel(PixUInt x, PixUInt y, uint32_t c, BlendMode b_mode) {
  _remap_for_orientation(&x, &y);
  if ((x < _x) && (y < _y)) {
    const uint32_t OFFSET = _pixel_offset(x, y);
    // Fast path: no blending requested and no alpha channel in destination.
    if ((BlendMode::REPLACE == b_mode) && (ImgBufferFormat::R8_G8_B8_ALPHA != _buf_fmt)) {
      switch (_buf_fmt) {
        case ImgBufferFormat::R3_G3_B2:          // 8-bit color
        case ImgBufferFormat::GREY_8:            // 8-bit greyscale
          *(_buffer + OFFSET) = (uint8_t) c;
          break;
        case ImgBufferFormat::R5_G6_B5:          // 16-bit color
        case ImgBufferFormat::GREY_16:           // 16-bit greyscale
          *((uint16_t*) (_buffer + OFFSET)) = (uint16_t) (endianFlip() ? (endianSwap32(c) >> 16) : c);
          break;
        case ImgBufferFormat::R8_G8_B8:          // 24-bit color
        case ImgBufferFormat::GREY_24:           // 24-bit greyscale
          {
            const uint32_t COLOR_BYTES = (endianFlip() ? (endianSwap32(c) >> 8) : c);
            memcpy((_buffer + OFFSET), (void*) &COLOR_BYTES, 3);
          }
          break;
        case ImgBufferFormat::R8_G8_B8_ALPHA:
          *((uint32_t*) (_buffer + OFFSET)) = (endianFlip() ? endianSwap32(c) : c);
          break;
        case ImgBufferFormat::MONOCHROME:        // Monochrome
          {
            const PixUInt term0 = (_y >> 3);
            const uint32_t offset = x * term0 + (term0 - (y >> 3) - 1);
            const uint8_t bit_mask = 1 << (7 - (y & 0x07));
            uint8_t byte_group = *(_buffer + offset);
            *(_buffer + offset) = (byte_group & ~bit_mask) | ((0 == c) ? 0 : bit_mask);
          }
          break;
        case ImgBufferFormat::UNALLOCATED:
        default:
          return false;
      }
      return true;
    }

    // Otherwise: need to blend (or dst has alpha).
    uint32_t d_raw = 0;
    switch (_buf_fmt) {
      case ImgBufferFormat::R8_G8_B8_ALPHA:
        if (endianFlip()) {
          d_raw  = ((uint32_t) *(_buffer + OFFSET + 3));
          d_raw |= ((uint32_t) *(_buffer + OFFSET + 2) << 8);
          d_raw |= ((uint32_t) *(_buffer + OFFSET + 1) << 16);
          d_raw |= ((uint32_t) *(_buffer + OFFSET + 0) << 24);
        }
        else {
          d_raw  = ((uint32_t) *(_buffer + OFFSET + 0));
          d_raw |= ((uint32_t) *(_buffer + OFFSET + 1) << 8);
          d_raw |= ((uint32_t) *(_buffer + OFFSET + 2) << 16);
          d_raw |= ((uint32_t) *(_buffer + OFFSET + 3) << 24);
        }
        break;

      case ImgBufferFormat::R8_G8_B8:
      case ImgBufferFormat::GREY_24:
        if (endianFlip()) {
          d_raw = ((uint32_t)*(_buffer + OFFSET + 0) << 16) | ((uint32_t)*(_buffer + OFFSET + 1) << 8) | (uint32_t)*(_buffer + OFFSET + 2);
        }
        else {
          d_raw = ((uint32_t)*(_buffer + OFFSET + 2) << 16) | ((uint32_t)*(_buffer + OFFSET + 1) << 8) | (uint32_t)*(_buffer + OFFSET + 0);
        }
        break;

      case ImgBufferFormat::R5_G6_B5:
      case ImgBufferFormat::GREY_16:
        if (endianFlip()) {
          d_raw = ((uint32_t) *(_buffer + OFFSET + 0) << 8) | (uint32_t) *(_buffer + OFFSET + 1);
        }
        else {
          d_raw = ((uint32_t) *(_buffer + OFFSET + 1) << 8) | (uint32_t) *(_buffer + OFFSET + 0);
        }
        break;

      case ImgBufferFormat::R3_G3_B2:
      case ImgBufferFormat::GREY_8:
        d_raw = (uint32_t) *(_buffer + OFFSET);
        break;

      case ImgBufferFormat::MONOCHROME:
        {
          const uint32_t term0 = (_y >> 3);
          const uint8_t bit_mask = 1 << (7 - (y & 0x07));
          const uint32_t mo = x * term0 + (term0 - (y >> 3) - 1);
          const uint8_t byte_group = *(_buffer + mo);
          d_raw = (byte_group & bit_mask) ? 0xFFFFFFFF : 0;
        }
        break;

      case ImgBufferFormat::UNALLOCATED:
      default:
        return false;
    }

    const bool DST_HAS_ALPHA = (ImgBufferFormat::R8_G8_B8_ALPHA == _buf_fmt);

    uint8_t sr = 0, sg = 0, sb = 0, sa = 0xFF;
    uint8_t dr = 0, dg = 0, db = 0, da = 0xFF;

    _unpack_rgba(c, _buf_fmt, &sr, &sg, &sb, &sa);
    _unpack_rgba(d_raw, _buf_fmt, &dr, &dg, &db, &da);

    if (BlendMode::REPLACE == b_mode) {
      // Unconditional clobber. If dst has alpha, preserve src alpha; else ignore it.
      const uint8_t OA = (DST_HAS_ALPHA ? sa : 0xFF);
      const uint32_t packed_copy = _pack_from_rgba(sr, sg, sb, OA, _buf_fmt);

      // Write packed pixel in native buffer representation.
      switch (_buf_fmt) {
        case ImgBufferFormat::R3_G3_B2:
        case ImgBufferFormat::GREY_8:
          *(_buffer + OFFSET) = (uint8_t) packed_copy;
          break;
        case ImgBufferFormat::R5_G6_B5:
        case ImgBufferFormat::GREY_16:
          *((uint16_t*) (_buffer + OFFSET)) = (uint16_t) (endianFlip() ? (endianSwap32(packed_copy) >> 16) : packed_copy);
          break;
        case ImgBufferFormat::R8_G8_B8_ALPHA:
          *((uint32_t*) (_buffer + OFFSET)) = (endianFlip() ? endianSwap32(packed_copy) : packed_copy);
          break;
        case ImgBufferFormat::R8_G8_B8:
        case ImgBufferFormat::GREY_24:
          {
            const uint32_t COLOR_BYTES = (endianFlip() ? (endianSwap32(packed_copy) >> 8) : packed_copy);
            memcpy((_buffer + OFFSET), (void*) &COLOR_BYTES, 3);
          }
          break;
        case ImgBufferFormat::MONOCHROME:
          {
            const PixUInt term0 = (_y >> 3);
            const uint32_t mo = x * term0 + (term0 - (y >> 3) - 1);
            const uint8_t bit_mask = 1 << (7 - (y & 0x07));
            uint8_t byte_group = *(_buffer + mo);
            *(_buffer + mo) = (byte_group & ~bit_mask) | ((0 == packed_copy) ? 0 : bit_mask);
          }
          break;
        case ImgBufferFormat::UNALLOCATED:
        default:
          return false;
      }
      return true;
    }

    if (!DST_HAS_ALPHA) {
      sa = 0xFF;
      da = 0xFF;
    }

    if (0 == sa) {
      return true;  // Source fully transparent => no-op (non-COPY only).
    }

    // Alpha application first: src contribution attenuated by sa.
    const uint8_t sra = _u8_mul_255(sr, sa);
    const uint8_t sga = _u8_mul_255(sg, sa);
    const uint8_t sba = _u8_mul_255(sb, sa);
    const uint8_t inv = (uint8_t) (255 - sa);

    uint8_t orr = dr;
    uint8_t org = dg;
    uint8_t orb = db;

    switch (b_mode) {
      case BlendMode::OVER:
        // Standard "over": out = src*sa + dst*(1-sa)
        orr = (uint8_t) ((((uint16_t) sr * (uint16_t) sa) + ((uint16_t) dr * (uint16_t) inv) + 127) / 255);
        org = (uint8_t) ((((uint16_t) sg * (uint16_t) sa) + ((uint16_t) dg * (uint16_t) inv) + 127) / 255);
        orb = (uint8_t) ((((uint16_t) sb * (uint16_t) sa) + ((uint16_t) db * (uint16_t) inv) + 127) / 255);
        break;

      case BlendMode::ADD_SAT:
        orr = _u8_sat_add(dr, sra);
        org = _u8_sat_add(dg, sga);
        orb = _u8_sat_add(db, sba);
        break;

      case BlendMode::SUB_SAT:
        orr = _u8_sat_sub(dr, sra);
        org = _u8_sat_sub(dg, sga);
        orb = _u8_sat_sub(db, sba);
        break;

      case BlendMode::SCALE:
        {
          // Factor = lerp(255, src, alpha) per channel: inv + src*sa/255
          const uint8_t fr = (uint8_t) _u8_sat_add(inv, sra);
          const uint8_t fg = (uint8_t) _u8_sat_add(inv, sga);
          const uint8_t fb = (uint8_t) _u8_sat_add(inv, sba);
          orr = _u8_mul_255(dr, fr);
          org = _u8_mul_255(dg, fg);
          orb = _u8_mul_255(db, fb);
        }
        break;

      default:
        // Treat unknown as NONE.
        orr = (uint8_t) ((((uint16_t) sr * (uint16_t) sa) + ((uint16_t) dr * (uint16_t) inv) + 127) / 255);
        org = (uint8_t) ((((uint16_t) sg * (uint16_t) sa) + ((uint16_t) dg * (uint16_t) inv) + 127) / 255);
        orb = (uint8_t) ((((uint16_t) sb * (uint16_t) sa) + ((uint16_t) db * (uint16_t) inv) + 127) / 255);
        break;
    }

    uint8_t oa = 0xFF;
    if (DST_HAS_ALPHA) {
      oa = (uint8_t) ((uint16_t) sa + (uint16_t) ((((uint16_t) da * (uint16_t) inv) + 127) / 255));
    }

    const uint32_t packed = _pack_from_rgba(orr, org, orb, oa, _buf_fmt);

    // Now write packed pixel in native buffer representation.
    switch (_buf_fmt) {
      case ImgBufferFormat::R3_G3_B2:
      case ImgBufferFormat::GREY_8:
        *(_buffer + OFFSET) = (uint8_t) packed;
        break;
      case ImgBufferFormat::R5_G6_B5:
      case ImgBufferFormat::GREY_16:
        *((uint16_t*) (_buffer + OFFSET)) = (uint16_t) (endianFlip() ? (endianSwap32(packed) >> 16) : packed);
        break;
      case ImgBufferFormat::R8_G8_B8_ALPHA:
        *((uint32_t*) (_buffer + OFFSET)) = (endianFlip() ? endianSwap32(packed) : packed);
        break;
      case ImgBufferFormat::R8_G8_B8:
      case ImgBufferFormat::GREY_24:
        {
          const uint32_t COLOR_BYTES = (endianFlip() ? (endianSwap32(packed) >> 8) : packed);
          memcpy((_buffer + OFFSET), (void*) &COLOR_BYTES, 3);
        }
        break;
      case ImgBufferFormat::MONOCHROME:
        {
          const PixUInt term0 = (_y >> 3);
          const uint32_t mo = x * term0 + (term0 - (y >> 3) - 1);
          const uint8_t bit_mask = 1 << (7 - (y & 0x07));
          uint8_t byte_group = *(_buffer + mo);
          *(_buffer + mo) = (byte_group & ~bit_mask) | ((0 == packed) ? 0 : bit_mask);
        }
        break;
      case ImgBufferFormat::UNALLOCATED:
      default:
        return false;
    }
    return true;
  }
  return false;
}


/**
*/
uint32_t Image::getPixel(PixUInt x, PixUInt y) {
  uint32_t ret = 0;
  uint32_t sz = bytesUsed();
  _remap_for_orientation(&x, &y);
  if ((x < _x) & (y < _y)) {
    uint32_t OFFSET = _pixel_offset(x, y);
    switch (_buf_fmt) {
      case ImgBufferFormat::R8_G8_B8_ALPHA:    // 32-bit color
        if (endianFlip()) {
          ret  = ((uint32_t) *(_buffer + OFFSET + 3));
          ret |= ((uint32_t) *(_buffer + OFFSET + 2) << 8);
          ret |= ((uint32_t) *(_buffer + OFFSET + 1) << 16);
          ret |= ((uint32_t) *(_buffer + OFFSET + 0) << 24);
        }
        else {
          ret  = ((uint32_t) *(_buffer + OFFSET + 0));
          ret |= ((uint32_t) *(_buffer + OFFSET + 1) << 8);
          ret |= ((uint32_t) *(_buffer + OFFSET + 2) << 16);
          ret |= ((uint32_t) *(_buffer + OFFSET + 3) << 24);
        }
        break;
      case ImgBufferFormat::GREY_24:           // 24-bit greyscale   TODO: Wrong. Has to be.
      case ImgBufferFormat::R8_G8_B8:          // 24-bit color
        if (endianFlip()) {
          ret = ((uint32_t)*(_buffer + OFFSET + 0) << 16) | ((uint32_t)*(_buffer + OFFSET + 1) << 8) | (uint32_t)*(_buffer + OFFSET + 2);
        }
        else {
          ret = ((uint32_t)*(_buffer + OFFSET + 2) << 16) | ((uint32_t)*(_buffer + OFFSET + 1) << 8) | (uint32_t)*(_buffer + OFFSET + 0);
        }
        break;
      case ImgBufferFormat::GREY_16:           // 16-bit greyscale   TODO: Wrong. Has to be.
      case ImgBufferFormat::R5_G6_B5:          // 16-bit color
        if (endianFlip()) {
          ret = ((uint32_t) *(_buffer + OFFSET + 0) << 8) | (uint32_t) *(_buffer + OFFSET + 1);
        }
        else {
          ret = ((uint32_t) *(_buffer + OFFSET + 1) << 8) | (uint32_t) *(_buffer + OFFSET + 0);
        }
        break;
      case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
      case ImgBufferFormat::R3_G3_B2:          // 8-bit color
        ret = (uint32_t) *(_buffer + OFFSET);
        break;
      case ImgBufferFormat::MONOCHROME:        // Monochrome
        {
          const uint32_t term0 = (_y >> 3);
          const uint8_t bit_mask = 1 << (7 - (y & 0x07));
          OFFSET = x * term0 + (term0 - (y >> 3) - 1);
          uint8_t byte_group = *(_buffer + OFFSET);
          // Full saturation on boolean pixel value.
          ret = (byte_group & bit_mask) ? 0xFFFFFFFF : 0;
        }
        break;
      case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
      default:
        break;
    }
  }
  return ret;
}



uint32_t Image::getPixelAsFormat(PixUInt x, PixUInt y, ImgBufferFormat target_fmt) {
  uint32_t c = getPixel(x, y);
  if (target_fmt == _buf_fmt) {
    return c;   // Nothing further to do.
  }
  uint8_t a = 0, r = 0, g = 0, b = 0;
  uint8_t source_bpc = 8;     // Used later for greyscale conversion.
  uint8_t target_bpp = 24;    // Used later for greyscale conversion.
  uint32_t ret = 0;
  switch (_buf_fmt) {
    case ImgBufferFormat::R8_G8_B8_ALPHA:    // 32-bit color
      r = (uint8_t) (c >> 24) & 0xFF;
      g = (uint8_t) (c >> 16) & 0xFF;
      b = (uint8_t) (c >> 8) & 0xFF;
      a = (uint8_t) (c & 0xFF);
      break;
    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
      r = (uint8_t) (c >> 16) & 0xFF;
      g = (uint8_t) (c >> 8) & 0xFF;
      b = (uint8_t) (c & 0xFF);
      break;
    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
      r = (uint8_t) ((c >> 11) & 0x1F) << 3;
      g = (uint8_t) ((c >> 5) & 0x3F) << 2;
      b = (uint8_t) (c & 0x1F) << 3;
      source_bpc = 6;
      break;
    case ImgBufferFormat::R3_G3_B2:          // 8-bit color
      r = (uint8_t) ((c >> 5) & 0x07) << 5;
      g = (uint8_t) ((c >> 2) & 0x07) << 5;
      b = (uint8_t) (c & 0x03) << 6;
      source_bpc = 3;
      break;
    case ImgBufferFormat::MONOCHROME:        // Monochrome
      if (c) {  a = 0xFF; r = 0xFF; g = 0xFF; b = 0xFF;   }
      else {    a = 0;    r = 0;    g = 0;    b = 0;      }
      break;
    case ImgBufferFormat::GREY_24:           // 24-bit greyscale   TODO: Wrong. Has to be.
    case ImgBufferFormat::GREY_16:           // 16-bit greyscale   TODO: Wrong. Has to be.
    case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
    case ImgBufferFormat::GREY_4:
    default:
      return ret;
  }

  switch (target_fmt) {
    case ImgBufferFormat::R8_G8_B8_ALPHA:    // 32-bit color
      ret |= ((uint32_t)r << 24);
      ret |= ((uint32_t)g << 16);
      ret |= ((uint32_t)b << 8);
      ret |= ((uint32_t)a);
      break;

    case ImgBufferFormat::R8_G8_B8:          // 24-bit color
      ret |= ((uint32_t)r << 16);
      ret |= ((uint32_t)g << 8);
      ret |= ((uint32_t)b);
      break;

    case ImgBufferFormat::R5_G6_B5:          // 16-bit color
      ret |= ((uint32_t)(r >> 3) << 11);
      ret |= ((uint32_t)(g >> 2) << 5);
      ret |= ((uint32_t)(b >> 3));
      break;
    case ImgBufferFormat::R3_G3_B2:          // 8-bit color
      ret |= ((uint32_t)(r >> 5) << 5);
      ret |= ((uint32_t)(g >> 5) << 2);
      ret |= ((uint32_t)(b >> 6));
      break;
    case ImgBufferFormat::GREY_4:            // 4-bit greyscale
      target_bpp -= 4;
    case ImgBufferFormat::GREY_8:            // 8-bit greyscale
      target_bpp -= 8;
    case ImgBufferFormat::GREY_16:           // 16-bit greyscale
      target_bpp -= 8;
    case ImgBufferFormat::GREY_24:           // 24-bit greyscale
      {
        // Conversion from color to grey or monochrome.
        float avg = ((r * 0.3) + (g * 0.59) + (b * 0.11)) / 3.0;
        float lum = avg / ((1 << source_bpc) - 1);
        ret = ((1 << target_bpp) - 1) * lum;
      }
      break;
    case ImgBufferFormat::MONOCHROME:        // Monochrome
      ret = (0 != c) ? 0xFFFFFFFF: 0;
      break;
    default:
      return 0;
  }
  return ret;
}


/**
* Takes a color in discrete RGB values. Squeezes it into the buffer's format, discarding low bits as appropriate.
*/
// Needless API complication. If this is really desired, it should be a shim that does
//   color conversion and calls setPixel(PixUInt x, PixUInt y, uint32_t c).
//bool Image::setPixel(PixUInt x, PixUInt y, uint8_t r, uint8_t g, uint8_t b) {
//  _remap_for_orientation(&x, &y);
//  uint32_t sz = bytesUsed();
//  uint32_t offset = _pixel_offset(x, y);
//  if (offset < sz) {
//    switch (_buf_fmt) {
//      case ImgBufferFormat::GREY_24:           // 24-bit greyscale   TODO: Wrong. Has to be.
//      case ImgBufferFormat::R8_G8_B8:          // 24-bit color
//        *(_buffer + offset + 0) = r;
//        *(_buffer + offset + 1) = g;
//        *(_buffer + offset + 2) = b;
//        break;
//      case ImgBufferFormat::GREY_16:           // 16-bit greyscale   TODO: Wrong. Has to be.
//      case ImgBufferFormat::R5_G6_B5:          // 16-bit color
//        *(_buffer + offset + 0) = ((uint16_t) ((r << 3) & 0xF8) << 8) | ((g >> 3) & 0x07);
//        *(_buffer + offset + 1) = ((uint16_t) ((g << 5) & 0xE0) << 8) | ((b >> 3) & 0x1F);
//        break;
//      case ImgBufferFormat::GREY_8:            // 8-bit greyscale    TODO: Wrong. Has to be.
//      case ImgBufferFormat::R3_G3_B2:          // 8-bit color
//        *(_buffer + offset) = ((uint8_t) (r << 6) & 0xE0) | ((uint8_t) (g << 2) & 0x1C) | (b & 0x07);
//        break;
//      case ImgBufferFormat::MONOCHROME:        // Monochrome
//        {
//          const PixUInt term0 = (_y >> 3);
//          const uint8_t bit_mask = 1 << (7 - (y & 0x07));
//          offset = x * term0 + (term0 - (y >> 3) - 1);
//          uint8_t byte_group = *(_buffer + offset);
//          *(_buffer + offset) = (byte_group & ~bit_mask) | ((0 == ((uint16_t) r + g + b)) ? 0 : bit_mask);
//        }
//        break;
//      case ImgBufferFormat::UNALLOCATED:       // Buffer unallocated
//      default:
//        return false;
//    }
//    return true;
//  }
//  else {
//    // Addressed pixel is out-of-bounds
//  }
//  return false;
//}


/**
* Alpha-composite another Image over this Image at the given destination origin.
* Source image is converted (per-pixel) into this Image's color space.
* Operation is clipped to this Image's bounds.
*
* @return true if arguments were valid (even if nothing overlapped), false on error.
*/
int8_t Image::blendImage(Image* src, const PixAddr TOP_LEFT, const BlendMode MODE) {
  if (!allocated()) { return -1; }
  if ((nullptr == src) || !src->allocated()) { return -2; }
  if (locked() || src->locked()) { return -3; }

  const PixUInt dst_x0 = TOP_LEFT.x;
  const PixUInt dst_y0 = TOP_LEFT.y;

  if ((dst_x0 >= _x) || (dst_y0 >= _y)) {
    return 0;   // Valid call, but nothing overlaps.
  }

  PixUInt copy_w = src->x();
  PixUInt copy_h = src->y();

  if ((PixUInt) (dst_x0 + copy_w) > _x) { copy_w = (PixUInt) (_x - dst_x0); }
  if ((PixUInt) (dst_y0 + copy_h) > _y) { copy_h = (PixUInt) (_y - dst_y0); }

  // If there's no overlap after clipping, we consider it a successful no-op.
  if ((0 == copy_w) || (0 == copy_h)) {
    return 0;
  }

  const ImgBufferFormat SRC_FMT = src->format();
  const ImgBufferFormat DST_FMT = _buf_fmt;
  const bool DST_HAS_ALPHA = (DST_FMT == ImgBufferFormat::R8_G8_B8_ALPHA);

  for (PixUInt sy = 0; sy < copy_h; sy++) {
    for (PixUInt sx = 0; sx < copy_w; sx++) {
      const PixUInt dx = (PixUInt) (dst_x0 + sx);
      const PixUInt dy = (PixUInt) (dst_y0 + sy);

      const uint32_t s_raw = src->getPixel(sx, sy);


      // Convert source pixel into destination format (color-space conversion lives in convertColor()).
      const uint32_t s_conv = convertColor(s_raw, SRC_FMT);

      uint8_t sr = 0, sg = 0, sb = 0, sa = 0xFF;
      _unpack_rgba(s_conv, DST_FMT, &sr, &sg, &sb, nullptr);

      // Source alpha comes from the *source* pixel encoding, not from the converted pixel.
      if (SRC_FMT == ImgBufferFormat::R8_G8_B8_ALPHA) {
        uint8_t tmp = 0;
        _unpack_rgba(s_raw, SRC_FMT, &tmp, &tmp, &tmp, &sa);
      }
      else {
        sa = 0xFF;
      }

      if (0 == sa) {
        continue;
      }

      const uint32_t d_raw = getPixel(dx, dy);
      uint8_t dr = 0, dg = 0, db = 0, da = 0xFF;
      _unpack_rgba(d_raw, DST_FMT, &dr, &dg, &db, &da);
      if (!DST_HAS_ALPHA) { da = 0xFF; }

      // Alpha application first: src contribution attenuated by sa.
      const uint8_t sra = _u8_mul_255(sr, sa);
      const uint8_t sga = _u8_mul_255(sg, sa);
      const uint8_t sba = _u8_mul_255(sb, sa);
      const uint8_t inv = (uint8_t) (255 - sa);

      uint8_t orr = dr;
      uint8_t org = dg;
      uint8_t orb = db;

      switch (MODE) {
        case BlendMode::REPLACE:
          orr = (uint8_t) ((((uint16_t) sr * (uint16_t) sa) + ((uint16_t) dr * (uint16_t) inv) + 127) / 255);
          org = (uint8_t) ((((uint16_t) sg * (uint16_t) sa) + ((uint16_t) dg * (uint16_t) inv) + 127) / 255);
          orb = (uint8_t) ((((uint16_t) sb * (uint16_t) sa) + ((uint16_t) db * (uint16_t) inv) + 127) / 255);
          break;

        case BlendMode::ADD_SAT:
          orr = _u8_sat_add(dr, sra);
          org = _u8_sat_add(dg, sga);
          orb = _u8_sat_add(db, sba);
          break;

        case BlendMode::SUB_SAT:
          orr = _u8_sat_sub(dr, sra);
          org = _u8_sat_sub(dg, sga);
          orb = _u8_sat_sub(db, sba);
          break;

        case BlendMode::SCALE:
          {
            const uint8_t fr = (uint8_t) _u8_sat_add(inv, sra);
            const uint8_t fg = (uint8_t) _u8_sat_add(inv, sga);
            const uint8_t fb = (uint8_t) _u8_sat_add(inv, sba);
            orr = _u8_mul_255(dr, fr);
            org = _u8_mul_255(dg, fg);
            orb = _u8_mul_255(db, fb);
          }
          break;

        default:
          orr = (uint8_t) ((((uint16_t) sr * (uint16_t) sa) + ((uint16_t) dr * (uint16_t) inv) + 127) / 255);
          org = (uint8_t) ((((uint16_t) sg * (uint16_t) sa) + ((uint16_t) dg * (uint16_t) inv) + 127) / 255);
          orb = (uint8_t) ((((uint16_t) sb * (uint16_t) sa) + ((uint16_t) db * (uint16_t) inv) + 127) / 255);
          break;
      }

      uint8_t oa = 0xFF;
      if (DST_HAS_ALPHA) {
        oa = (uint8_t) ((uint16_t) sa + (uint16_t) ((((uint16_t) da * (uint16_t) inv) + 127) / 255));
      }

      const uint32_t packed = _pack_from_rgba(orr, org, orb, oa, DST_FMT);
      setPixel(dx, dy, packed, BlendMode::REPLACE);
    }
  }
  return 0;
}


/*******************************************************************************
* Code below this block was ported from Adafruit's GFX library.
*******************************************************************************/

/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
    @param    size  Font magnification level, 1 is 'original' size
*/
void Image::drawChar(PixUInt x, PixUInt y, unsigned char c, uint32_t color, uint32_t bg, uint8_t size) {
  if (!_gfxFont) { // 'Classic' built-in font
    if ((x >= _x)  || // Clip right
       (y >= _y)   || // Clip bottom
           ((int32_t)(x + 6 * size - 1) < 0) || // Clip left
           ((int32_t)(y + 8 * size - 1) < 0))   // Clip top
            return;

        if(!cp437() && (c >= 176)) c++; // Handle 'classic' charset behavior

        _lock(true);
        for (int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
          uint8_t line = _default_font[c * 5 + i];
          for (int8_t j=0; j<8; j++, line >>= 1) {
            if(line & 1) {
              if(size == 1) {
                setPixel(x+i, y+j, color);
              }
              else {
                fillRect(x+i*size, y+j*size, size, size, color);
              }
            }
            else if(bg != color) {
              if (size == 1) {
                setPixel(x+i, y+j, bg);
              }
              else {
                fillRect(x+i*size, y+j*size, size, size, bg);
              }
            }
          }
        }
        if (bg != color) { // If opaque, draw vertical line for last column
            if(size == 1) drawFastVLine(x+5, y, 8, bg);
            else          fillRect(x+5*size, y, size, 8*size, bg);
        }
        _lock(false);
    }
    else { // Custom font
        // Character is assumed previously filtered by write() to eliminate
        // newlines, returns, non-printable characters, etc.  Calling
        // drawChar() directly with 'bad' characters of font may cause mayhem!
        c -= (uint8_t) _gfxFont->first;
        GFXglyph* glyph  = &(((GFXglyph*) _gfxFont->glyph))[c];
        uint8_t*  bitmap = (uint8_t*) _gfxFont->bitmap;
        uint16_t bo = glyph->bitmapOffset;
        uint8_t  w  = glyph->width;
        uint8_t  h  = glyph->height;
        int8_t   xo = glyph->xOffset;
        int8_t   yo = glyph->yOffset;
        uint8_t  xx, yy, bits = 0, bit = 0;
        int16_t  xo16 = 0, yo16 = 0;

        if (size > 1) {
          xo16 = xo;
          yo16 = yo;
        }
        // Todo: Add character clipping here

        // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
        // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
        // has typically been used with the 'classic' font to overwrite old
        // screen contents with new data.  This ONLY works because the
        // characters are a uniform size; it's not a sensible thing to do with
        // proportionally-spaced fonts with glyphs of varying sizes (and that
        // may overlap).  To replace previously-drawn text when using a custom
        // font, use the getTextBounds() function to determine the smallest
        // rectangle encompassing a string, erase the area with fillRect(),
        // then draw new text.  This WILL infortunately 'blink' the text, but
        // is unavoidable.  Drawing 'background' pixels will NOT fix this,
        // only creates a new set of problems.  Have an idea to work around
        // this (a canvas object type for MCUs that can afford the RAM and
        // displays supporting setAddrWindow() and pushColors()), but haven't
        // implemented this yet.

    _lock(true);
    for (yy=0; yy<h; yy++) {
      for (xx=0; xx<w; xx++) {
        if (!(bit++ & 7)) {
          bits = bitmap[bo++];
        }
        if (bits & 0x80) {
          if (size == 1) {
            setPixel(x+xo+xx, y+yo+yy, color);
          }
          else {
            fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
        }
        bits <<= 1;
      }
    }
    _lock(false);
  } // End classic vs custom font
}


uint16_t Image::getFontWidth() {
  return (_gfxFont) ? (_gfxFont->glyph->xAdvance * (uint16_t) _textsize) : ((uint16_t) _textsize * 6);
}


uint16_t Image::getFontHeight() {
  return (_gfxFont) ? ((uint16_t) _textsize * (uint8_t) _gfxFont->yAdvance) : ((uint16_t) _textsize * 8);
}


/*!
    @brief  Print one byte/character of data, used to support print()
    @param  c  The 8-bit ascii character to write
*/
void Image::writeChar(uint8_t c) {
  if (!_gfxFont) { // 'Classic' built-in font
    switch (c) {
      case '\n':                      // Newline?
        _cursor_y += _textsize * 8;         // advance y one line
        _cursor_x  = 0;                     // Reset x to zero,
        break;
      case '\r':                  // Ignore carriage returns
        break;
      case '\t':                  // Tab reduces to two spaces.
        c = ' ';
        if (textWrap() && ((_cursor_x + _textsize * 6) > _x)) { // Off right?
          _cursor_x  = 0;                 // Reset x to zero,
          _cursor_y += _textsize * 8;      // advance y one line
        }
        drawChar(_cursor_x, _cursor_y, c, _textcolor, _textbgcolor, _textsize);
        _cursor_x += _textsize * 6;
      default:
        if (textWrap() && ((_cursor_x + _textsize * 6) > _x)) { // Off right?
          _cursor_x  = 0;                 // Reset x to zero,
          _cursor_y += _textsize * 8;      // advance y one line
        }
        drawChar(_cursor_x, _cursor_y, c, _textcolor, _textbgcolor, _textsize);
        _cursor_x += _textsize * 6;          // Advance x one char
        break;
    }
  }
  else { // Custom font
    if (c == '\n') {
      _cursor_x  = 0;
      _cursor_y += (int16_t) _textsize * (uint8_t) _gfxFont->yAdvance;
    }
    else if (c != '\r') {
      uint8_t first = _gfxFont->first;
      if ((c >= first) && (c <= _gfxFont->last)) {
        GFXglyph* glyph = &_gfxFont->glyph[c - first];
        uint8_t  w      = glyph->width;
        uint8_t  h      = glyph->height;
        if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
          int16_t xo = (int8_t) glyph->xOffset; // sic
          if (textWrap() && ((_cursor_x + _textsize * (xo + w)) > _x)) {
            _cursor_x  = 0;
            _cursor_y += (int16_t) _textsize * (uint8_t)_gfxFont->yAdvance;
          }
          drawChar(_cursor_x, _cursor_y, c, _textcolor, _textbgcolor, _textsize);
        }
        _cursor_x += (uint8_t) glyph->xAdvance * (int16_t) _textsize;
      }
    }
  }
}


void Image::writeString(StringBuilder* str) {
  writeString((const char*) str->string());
}


void Image::writeString(const char* str) {
  int len = strlen(str);
  for (int i = 0; i < len; i++) {
    writeChar((uint8_t) *(str + i));
  }
}


/*!
  @brief  Set text cursor location
  @param  x    X coordinate in pixels
  @param  y    Y coordinate in pixels
*/
void Image::setCursor(PixUInt x, PixUInt y) {
  _cursor_x = x;
  _cursor_y = y;
}

/*!
  @brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
  @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
void Image::setTextSize(uint8_t s) {
  _textsize = (s > 0) ? s : 1;
}

/*!
  @brief   Set text font color with transparant background
  @param   c
*/
void Image::setTextColor(uint32_t c) {
  // For 'transparent' background, we'll set the bg
  // to the same as fg instead of using a flag
  _textcolor = _textbgcolor = c;
}


/*!
  @brief   Set text font color and background
  @param   c
*/
void Image::setTextColor(uint32_t c, uint32_t bg) {
  _textcolor   = c;
  _textbgcolor = bg;
}


/*!
  @brief    Helper to determine size of a character with current font/size.
     Broke this out as it's used by both the PROGMEM- and RAM-resident getTextBounds() functions.
  @param    c     The ascii character in question
  @param    x     Pointer to x location of character
  @param    y     Pointer to y location of character
  @param    minx  Minimum clipping value for X
  @param    miny  Minimum clipping value for Y
  @param    maxx  Maximum clipping value for X
  @param    maxy  Maximum clipping value for Y
*/
void Image::_char_bounds(char c, PixUInt* x, PixUInt* y, PixUInt* minx, PixUInt* miny, PixUInt* maxx, PixUInt* maxy) {
  if (_gfxFont) {
    if (c == '\n') { // Newline?
      *x  = 0;    // Reset x to zero, advance y by one line
      *y += _textsize * (uint8_t)_gfxFont->yAdvance;
    }
    else if (c != '\r') { // Not a carriage return; is normal char
      uint8_t first = _gfxFont->first;
      uint8_t last  = _gfxFont->last;
      if((c >= first) && (c <= last)) { // Char present in this font?
        GFXglyph* glyph = &(((GFXglyph*) _gfxFont->glyph))[c - first];
        uint8_t gw = glyph->width;
        uint8_t gh = glyph->height;
        uint8_t xa = glyph->xAdvance;
        int8_t  xo = glyph->xOffset;
        int8_t  yo = glyph->yOffset;
        if(textWrap() && ((*x+(((int16_t)xo+gw) * _textsize)) > _x)) {
          *x  = 0; // Reset x to zero, advance y by one line
          *y += _textsize * (uint8_t) _gfxFont->yAdvance;
        }
        int16_t ts = (int16_t) _textsize;
        PixUInt x1 = *x + xo * ts;
        PixUInt y1 = *y + yo * ts;
        PixUInt x2 = x1 + gw * ts - 1;
        PixUInt y2 = y1 + gh * ts - 1;
        if(x1 < *minx) *minx = x1;
        if(y1 < *miny) *miny = y1;
        if(x2 > *maxx) *maxx = x2;
        if(y2 > *maxy) *maxy = y2;
        *x += xa * ts;
      }
    }
  }
  else { // Default font
    if(c == '\n') {                   // Newline?
      *x  = 0;                        // Reset x to zero,
      *y += _textsize * 8;             // advance y one line
      // min/max x/y unchaged -- that waits for next 'normal' character
    }
    else if (c != '\r') {  // Normal char; ignore carriage returns
      if (textWrap() && ((*x + _textsize * 6) > _x)) { // Off right?
        *x  = 0;                    // Reset x to zero,
        *y += _textsize * 8;         // advance y one line
      }
      int32_t x2 = *x + _textsize * 6 - 1; // Lower-right pixel of char
      int32_t y2 = *y + _textsize * 8 - 1;
      if(x2 > (int32_t) *maxx) {  *maxx = x2;  }      // Track max x, y
      if(y2 > (int32_t) *maxy) {  *maxy = y2;  }
      if(*x < *minx) {  *minx = *x;  }      // Track min x, y
      if(*y < *miny) {  *miny = *y;  }
      *x += _textsize * 6;             // Advance x one char
    }
  }
}


/*!
  @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
  @param    str     The ascii string to measure
  @param    x       The current cursor X
  @param    y       The current cursor Y
  @param    x1      The boundary X coordinate, set by function
  @param    y1      The boundary Y coordinate, set by function
  @param    w      The boundary width, set by function
  @param    h      The boundary height, set by function
*/
void Image::getTextBounds(const char* str, PixUInt x, PixUInt y, PixUInt* x1, PixUInt* y1, PixUInt* w, PixUInt* h) {
  uint8_t c; // Current character
  *x1 = x;
  *y1 = y;
  *w  = *h = 0;

  PixUInt minx = _x;
  PixUInt miny = _y;
  PixUInt maxx = 0;
  PixUInt maxy = 0;

  while((c = *str++)) {
    _char_bounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  }

  if(maxx >= minx) {
    *x1 = minx;
    *w  = maxx - minx + 1;
  }
  if(maxy >= miny) {
    *y1 = miny;
    *h  = maxy - miny + 1;
  }
}


/*!
  @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
  @param    str    The ascii string to measure (as an arduino String() class)
  @param    x      The current cursor X
  @param    y      The current cursor Y
  @param    x1     The boundary X coordinate, set by function
  @param    y1     The boundary Y coordinate, set by function
  @param    w      The boundary width, set by function
  @param    h      The boundary height, set by function
*/
void Image::getTextBounds(StringBuilder* str, PixUInt x, PixUInt y, PixUInt* x1, PixUInt* y1, PixUInt* w, PixUInt* h) {
  if (str->length() != 0) {
    getTextBounds((const uint8_t*) str->string(), x, y, x1, y1, w, h);
  }
}


/*!
  @brief    Helper to determine size of a PROGMEM string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
  @param    str     The flash-memory ascii string to measure
  @param    x       The current cursor X
  @param    y       The current cursor Y
  @param    x1      The boundary X coordinate, set by function
  @param    y1      The boundary Y coordinate, set by function
  @param    w      The boundary width, set by function
  @param    h      The boundary height, set by function
*/
void Image::getTextBounds(const uint8_t* str, PixUInt x, PixUInt y, PixUInt* x1, PixUInt* y1, PixUInt* w, PixUInt* h) {
  uint8_t* s = (uint8_t*) str;
  uint8_t c;
  *x1 = x;
  *y1 = y;
  *w  = *h = 0;

  PixUInt minx = _x;
  PixUInt miny = _y;
  PixUInt maxx = -1;
  PixUInt maxy = -1;

  while((c = *s++)) {
    _char_bounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
  }

  if(maxx >= minx) {
    *x1 = minx;
    *w  = maxx - minx + 1;
  }
  if (maxy >= miny) {
    *y1 = miny;
    *h  = maxy - miny + 1;
  }
}



/*!
  @brief    Write a perfectly vertical line, overwrite in subclasses if startWrite is defined!
  @param    x   Top-most x coordinate
  @param    y   Top-most y coordinate
  @param    h   Height in pixels
  @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::drawFastVLine(PixUInt x, PixUInt y, PixUInt h, uint32_t color) {
  // Overwrite in subclasses if startWrite is defined!
  // Can be just drawLine(x, y, x, y+h-1, color);
  // or fillRect(x, y, 1, h, color);
  _lock(true);
  drawLine(x, y, x, y+h-1, color);
  _lock(false);
}


/*!
  @brief    Write a perfectly horizontal line, overwrite in subclasses if startWrite is defined!
  @param    x   Left-most x coordinate
  @param    y   Left-most y coordinate
  @param    w   Width in pixels
  @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::drawFastHLine(PixUInt x, PixUInt y, PixUInt w, uint32_t color) {
  // Overwrite in subclasses if startWrite is defined!
  // Example: drawLine(x, y, x+w-1, y, color);
  // or fillRect(x, y, w, 1, color);
  _lock(true);
  drawLine(x, y, x+w-1, y, color);
  _lock(false);
}


/*!
  @brief    Fill a rectangle completely with one color. Update in subclasses if desired!
  @param    x   Top left corner x coordinate
  @param    y   Top left corner y coordinate
  @param    w   Width in pixels
  @param    h   Height in pixels
  @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::fillRect(PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color) {
  _lock(true);
  for (uint32_t i=x; i<x+w; i++) {
    drawLine(i, y, i, y+h-1, color);
  }
  _lock(false);
}


/**
* @brief   Draw a rectangle with no fill color
*  @param    x   Top left corner x coordinate
*  @param    y   Top left corner y coordinate
*  @param    w   Width in pixels
*  @param    h   Height in pixels
*  @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawRect(PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

/**
   @brief   Draw a rounded rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawRoundRect(PixUInt x, PixUInt y, PixUInt w, PixUInt h, PixUInt r, uint32_t color) {
  PixUInt max_radius = ((w < h) ? w : h) >> 1; // 1/2 minor axis
  if (r > max_radius) r = max_radius;
  // smarter version
  drawFastHLine(x + r, y, w - 2 * r, color);         // Top
  drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
  drawFastVLine(x, y + r, h - 2 * r, color);         // Left
  drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
  // draw four corners
  drawCircleHelper(x + r, y + r, r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}


/*!
   @brief   Draw a rounded rectangle with fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw/fill with
*/
void Image::fillRoundRect(PixUInt x, PixUInt y, PixUInt w, PixUInt h, PixUInt r, uint32_t color) {
  PixUInt max_radius = ((w < h) ? w : h) >> 1; // 1/2 minor axis
  if (r > max_radius) r = max_radius;
  // smarter version
  fillRect(x + r, y, w - 2 * r, h, color);
  // draw four corners
  fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}


/*!
   @brief   Draw a triangle with no fill color
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawTriangle(PixUInt x0, PixUInt y0, PixUInt x1, PixUInt y1, PixUInt x2, PixUInt y2, uint32_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}


/*!
   @brief     Draw a triangle with color-fill
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to fill/draw with
*/
void Image::fillTriangle(PixUInt x0, PixUInt y0, PixUInt x1, PixUInt y1, PixUInt x2, PixUInt y2, uint32_t color) {
  PixUInt a, b, y, last;
  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    strict_swap(&y0, &y1);
    strict_swap(&x0, &x1);
  }
  if (y1 > y2) {
    strict_swap(&y2, &y1);
    strict_swap(&x2, &x1);
  }
  if (y0 > y1) {
    strict_swap(&y0, &y1);
    strict_swap(&x0, &x1);
  }

  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a) {       a = x1;   }
    else if (x1 > b) {  b = x1;   }
    if (x2 < a) {       a = x2;   }
    else if (x2 > b) {  b = x2;   }
    drawFastHLine(a, y0, b - a + 1, color);
    return;
  }

  PixUInt dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
          dx12 = x2 - x1, dy12 = y2 - y1;
  PixUInt sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2) {    last = y1;       }  // Include y1 scanline
  else {             last = y1 - 1;   }  // Skip it

  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b) {  strict_swap(&a, &b);   }
    drawFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b) {  strict_swap(&a, &b);   }
    drawFastHLine(a, y, b - a + 1, color);
  }
}


/*!
   @brief    Draw a circle outline
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawCircle(PixUInt x0, PixUInt y0, PixUInt r, uint32_t color) {
  int32_t f = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * r;
  int32_t x = 0;
  int32_t y = r;

  setPixel(x0, y0 + r, color);
  setPixel(x0, y0 - r, color);
  setPixel(x0 + r, y0, color);
  setPixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    setPixel(x0 + x, y0 + y, color);
    setPixel(x0 - x, y0 + y, color);
    setPixel(x0 + x, y0 - y, color);
    setPixel(x0 - x, y0 - y, color);
    setPixel(x0 + y, y0 + x, color);
    setPixel(x0 - y, y0 + x, color);
    setPixel(x0 + y, y0 - x, color);
    setPixel(x0 - y, y0 - x, color);
  }
}


/*!
   @brief    Draw an ellipse outline
    @param    x0       Center-point x coordinate
    @param    y0       Center-point y coordinate
    @param    v_axis   Radius on y coordinate.
    @param    h_axis   Radius on x coordinate.
    @param    rotation Rotation (in degrees) clockwise about the center.
    @param    color    Color to draw with
*/
void Image::drawEllipse(PixUInt x0, PixUInt y0, PixUInt v_axis, PixUInt h_axis, float rotation, uint32_t color) {
  if (v_axis == h_axis) {  // Is this an over-specified circle?
    drawCircle(x0, y0, h_axis, color);
  }
  else {
    float radians = (PI/180.0) * rotation;
    //int32_t vertical_extent   = abs(sin(radians)) * v_axis;
    int32_t horizontal_extent = abs(cos(radians)) * h_axis;
    int32_t starting_x = strict_max(0,             (int32_t) (x0 - horizontal_extent));
    int32_t ending_x   = strict_min((int32_t) x(), (int32_t) (x0 + horizontal_extent));
    for (int32_t i = starting_x; i < ending_x; i++) {
      PixUInt a = h_axis;
      PixUInt b = v_axis;
      float    y_diff   = (b / a) * sqrt((a*a) - (i*i));
      int32_t  y_top    = y0 - y_diff;
      int32_t  y_bottom = y0 + y_diff;
      if (y_top >= 0) {
        setPixel(i, y_top, color);
      }
      if (y_bottom < (int32_t) y()) {
        setPixel(i, y_bottom, color);
      }
    }
  }
}


/*!
    @brief    Quarter-circle drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    quadrant  Mask bit #1 or bit #2 to indicate which quarters of
   the circle we're doing
    @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawCircleHelper(PixUInt x0, PixUInt y0, PixUInt r, uint8_t quadrant, uint32_t color) {
  int32_t f = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * r;
  int32_t x = 0;
  int32_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (quadrant & 0x4) {
      setPixel(x0 + x, y0 + y, color);
      setPixel(x0 + y, y0 + x, color);
    }
    if (quadrant & 0x2) {
      setPixel(x0 + x, y0 - y, color);
      setPixel(x0 + y, y0 - x, color);
    }
    if (quadrant & 0x8) {
      setPixel(x0 - y, y0 + x, color);
      setPixel(x0 - x, y0 + y, color);
    }
    if (quadrant & 0x1) {
      setPixel(x0 - y, y0 - x, color);
      setPixel(x0 - x, y0 - y, color);
    }
  }
}


/*!
   @brief    Draw a circle with filled color
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to fill with
*/
void Image::fillCircle(PixUInt x0, PixUInt y0, PixUInt r, uint32_t color) {
  drawFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

/*!
    @brief  Quarter-circle drawer with fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to fill with
*/
void Image::fillCircleHelper(PixUInt x0, PixUInt y0, PixUInt r, uint8_t corners, PixUInt delta, uint32_t color) {
  int32_t f = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * r;
  int32_t x = 0;
  int32_t y = r;
  int32_t px = x;
  int32_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}


/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y)
   position. For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
void Image::drawBitmap(PixUInt x, PixUInt y, const uint8_t* bitmap, PixUInt w, PixUInt h) {
  for (PixUInt j = 0; j < h; j++, y++) {
    for (PixUInt i = 0; i < w; i++) {
      setPixel(x + i, y, *(bitmap + (j * w + i)));
    }
  }
}


/**
*
*/
void Image::fill(uint32_t color) {
  //uint8_t bpp = _bits_per_pixel();
  _lock(true);
  switch (_bits_per_pixel()) {
    case 32:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *((uint32_t*) (_buffer + (i << 2))) = (uint32_t) color;
      }
      break;
    case 24:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *(_buffer + (i << 2) + 0) = (uint8_t) (color >> 16) & 0xFF;
        *(_buffer + (i << 2) + 1) = (uint8_t) (color >> 8) & 0xFF;
        *(_buffer + (i << 2) + 2) = (uint8_t) (color & 0xFF);
      }
      break;
    case 16:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *((uint16_t*) (_buffer + (i << 1))) = (uint16_t) color;
      }
      break;
    case 8:
      for (uint32_t i=0; i < (_x * _y); i++) {
        *(_buffer + i) = (uint8_t) color;
      }
      break;
    case 1:   // NOTE: Assumes a pixel count of mod(8).
      for (uint32_t i=0; i < (uint32_t) ((_x * _y) >> 3); i++) {
        *(_buffer + i) = (0 != color) ? 0xFF : 0x00;
      }
      break;
  }
  _lock(false);
}


/*!
  @brief    Write a line.  Bresenham's algorithm - thx wikpedia
  @param    x0  Start point x coordinate
  @param    y0  Start point y coordinate
  @param    x1  End point x coordinate
  @param    y1  End point y coordinate
  @param    color 16-bit 5-6-5 Color to draw with
*/
void Image::drawLine(PixUInt x0, PixUInt y0, PixUInt x1, PixUInt y1, uint32_t color) {
  const bool steep = strict_abs_delta(y1, y0) > strict_abs_delta(x1, x0);
  if (steep) {
    strict_swap(&x0, &y0);
    strict_swap(&x1, &y1);
  }
  if (x0 > x1) {
    strict_swap(&x0, &x1);
    strict_swap(&y0, &y1);
  }

  const PixUInt dx    = x1 - x0;
  const PixUInt dy    = strict_abs_delta(y1, y0);
  const int8_t   ystep = (y0 < y1) ? 1 : -1;
  int32_t err = (int32_t) (dx >> 1);  // NOTE: Imposes width limit of 2,147,483,648 pixels.

  while (x0 <= x1) {
    if (steep) {
      setPixel(y0, x0, color);
    }
    else {
      setPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
    x0++;
  }
}

#endif // CONFIG_C3P_IMG_SUPPORT
