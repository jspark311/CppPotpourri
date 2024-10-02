/*
File:   Image.h
Author: J. Ian Lindsay
Date:   2019.06.02

A color-aware two-dimentional array, with support functions.

TODO: Default X/Y range to uint16 instead of uint32.
  As neat and tidy as it would be to support giga-pixel x/y coordinates, it
  is definitely surplus to requirements. Ease the storage burden by converting
  all the coordinates into a settable integer type. Big project...

Certain drawing features of this class were lifted from Adafruit's GFX library.
  Rather than call out which specific functions, or isolate translation units
  for legal reasons, this class simply inherrits their license and attribution
  unless I rework it. License reproduced in the comment block below this one.
                                                    ---J. Ian Lindsay 2019.06.14
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

#ifndef __C3P_TYPE_IMG_H
#define __C3P_TYPE_IMG_H

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include "../Meta/Rationalizer.h"
#include "../EnumeratedTypeCodes.h"
#include "../CppPotpourri.h"
#include "../StringBuilder.h"

class Image;

// Once the bit-width is defined, assign properly-sized integer types to our
//   wrapped types.
// TODO: PixInt might be wrong-headed. Might be better to just up-case to int32
//   while doing signed arithmetic on PixUInt (which is the primary type).
#if(9 > CONFIG_C3P_IMG_COORD_BITS)
  typedef uint8_t PixUInt;
  typedef int8_t  PixInt;
#elif(17 > CONFIG_C3P_IMG_COORD_BITS)
  typedef uint16_t PixUInt;
  typedef int16_t  PixInt;
#elif(33 > CONFIG_C3P_IMG_COORD_BITS)
  typedef uint32_t PixUInt;
  typedef int32_t  PixInt;
#else
  #error Integers for holding pixel addresses cannot be larger than 32-bit.
#endif


// Font data stored PER GLYPH
// Taken from Adafruit GFX
typedef struct {
  uint16_t bitmapOffset; // Pointer into GFXfont->bitmap
  uint8_t  width;        // Bitmap dimensions in pixels
  uint8_t  height;       // Bitmap dimensions in pixels
  uint8_t  xAdvance;     // Distance to advance cursor (x axis)
  int8_t   xOffset;      // X dist from cursor pos to UL corner
  int8_t   yOffset;      // Y dist from cursor pos to UL corner
} GFXglyph;

// Data stored for FONT AS A WHOLE
// Taken from Adafruit GFX
typedef struct {
  uint8_t*  bitmap;      // Glyph bitmaps, concatenated
  GFXglyph* glyph;       // Glyph array
  uint8_t   first;       // ASCII extents (first char)
  uint8_t   last;        // ASCII extents (last char)
  uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;


enum class ImgBufferFormat : uint8_t {
  UNALLOCATED    = 0x00,  // Buffer unallocated
  MONOCHROME     = 0x01,  // Monochrome
  GREY_24        = 0x02,  // 24-bit greyscale
  GREY_16        = 0x03,  // 16-bit greyscale
  GREY_8         = 0x04,  // 8-bit greyscale
  GREY_4         = 0x05,  // 4-bit greyscale
  R8_G8_B8_ALPHA = 0x06,  // 24-bit color with 8-bits of alpha.
  R8_G8_B8       = 0x07,  // 24-bit color
  R5_G6_B5       = 0x08,  // 16-bit color
  R3_G3_B2       = 0x09   // 8-bit color
};

enum class ImgOrientation : uint8_t {
  ROTATION_0   = 0x00,  // 0-degrees
  ROTATION_90  = 0x01,  // 90-degrees
  ROTATION_180 = 0x02,  // 180-degrees
  ROTATION_270 = 0x03   // 270-degrees
};



/*******************************************************************************
* Coordinate types
*******************************************************************************/

/* A representation of a pixel address. */
class PixAddr {
  public:
    PixUInt x;   // X-coordinate (horizonal axis, larger values are further right)
    PixUInt y;   // Y-coordinate (vertical axis, larger values are further down)

    PixAddr(const PixAddr& src) : x(src.x), y(src.y) {};    // Copy-constructor
    PixAddr(const PixUInt X = 0, const PixUInt Y = 0) : x(X), y(Y) {};

    // TODO: Add some basic point functions?
    // PixInt distanceTo(const PixAddr);
};

/* A representation of an area within an Image. */
class ImageSubframe {
  public:
    ImageSubframe(const ImageSubframe& src) : _img(src._img), _addr(src._addr), _width(src._width), _height(src._height) {};   // Copy-constructor
    ImageSubframe(Image* i, const PixAddr A, const PixUInt W, const PixUInt H) : _img(i), _addr(A), _width(W), _height(H) {};
    ImageSubframe() : ImageSubframe(nullptr, (0, 0), 0, 0) {};

    // Pass an optional OFFSET value to arrive at an absolute location.
    PixUInt extentX(const PixUInt OFFSET = 0) {       return (OFFSET + _addr.x);  };
    PixUInt extentY(const PixUInt OFFSET = 0) {       return (OFFSET + _addr.y);  };
    PixAddr extent() {  return PixAddr((_width + _addr.x), (_height + _addr.y));  };

  protected:
    Image*  _img;            // Target Image
    PixAddr _addr;   // The pixel in the target image that is our upper-left corner.
    PixUInt _width;  // The width of this frame.
    PixUInt _height; // The height of this frame.
};



/*
* Class flags
* NOTE: The first byte is special, and forms the input to a switch/case demux
*   for the sake of transforming between coordinates and buffer indicies.
*/
#define C3P_IMG_FLAG_ORIENTATION_MASK 0x0003  // The lowest 2-bits hold the orientation enum.
#define C3P_IMG_FLAG_FORMAT_MASK      0x003C  // Mask for the image format.
#define C3P_IMG_FLAG_FLIP_X           0x0040  // If set, flip along the X-axis.
#define C3P_IMG_FLAG_FLIP_Y           0x0080  // If set, flip along the Y-axis.
#define C3P_IMG_FLAG_TXT_WRAPS        0x0100  // If set, text wraps at the right edge of the frame.
#define C3P_IMG_FLAG_TXT_WRAPS_TO_0   0x0200  // If set, when text wraps, it wraps to the left-edge of the frame.
#define C3P_IMG_FLAG_IS_STRICT_CP437  0x0400  // If set, use correct CP437 charset (default is off).
#define C3P_IMG_FLAG_BUFFER_OURS      0x0800  // We are responsible for freeing the buffer.
#define C3P_IMG_FLAG_BUFFER_LOCKED    0x1000  // Buffer should not be modified when set.
#define C3P_IMG_FLAG_IS_FRAMEBUFFER   0x2000  // This class holds the framebuffer for some piece of hardware.
#define C3P_IMG_FLAG_IS_FB_DIRTY      0x4000  // This image is dirty for the purposes of rendering.
#define C3P_IMG_FLAG_ENDIAN_FLIP      0x8000  // If set, perform an endian swap on the pixel data when writing.


/*******************************************************************************
* Our wrapper class for image data.
*******************************************************************************/
class Image {
  public:
    Image(PixUInt x, PixUInt y, ImgBufferFormat, uint8_t*);
    Image(PixUInt x, PixUInt y, ImgBufferFormat);
    Image(PixUInt x, PixUInt y);
    Image();
    ~Image();

    bool setBuffer(uint8_t*);
    bool setBuffer(uint8_t*, ImgBufferFormat);
    bool setBufferByCopy(uint8_t*);
    bool setBufferByCopy(uint8_t*, ImgBufferFormat);
    bool setSize(PixUInt x, PixUInt y);
    bool reallocate();
    uint32_t bytesUsed();

    void wipe();
    int8_t serialize(StringBuilder*);
    int8_t serialize(uint8_t*, uint32_t*);
    int8_t serializeWithoutBuffer(uint8_t*, uint32_t*);
    int8_t deserialize(uint8_t*, uint32_t);
    void   printImageInfo(StringBuilder*, const bool DETAIL = false);

    bool isColor();
    uint32_t convertColor(uint32_t color, ImgBufferFormat);
    inline uint32_t convertColor(uint32_t color) {    return convertColor(color, _buf_fmt);    };

    uint32_t getPixel(PixUInt x, PixUInt y);
    uint32_t getPixelAsFormat(PixUInt x, PixUInt y, ImgBufferFormat);
    bool     setPixel(PixUInt x, PixUInt y, uint32_t color);
    //bool     setPixel(PixUInt x, PixUInt y, uint8_t r, uint8_t g, uint8_t b);

    inline PixUInt         x() {              return _x;                                     };
    inline PixUInt         y() {              return _y;                                     };
    inline uint8_t*        buffer() {         return _buffer;                                };
    inline ImgBufferFormat format() {         return _buf_fmt;                               };
    inline bool            allocated() {      return (nullptr != _buffer);                   };
    inline uint32_t        pixels() {         return (_x * _y);                              };

    inline uint8_t         bitsPerPixel() {   return _bits_per_pixel(_buf_fmt);              };
    inline bool            isFrameBuffer() {  return _img_flag(C3P_IMG_FLAG_IS_FRAMEBUFFER); };
    inline bool            locked() {         return _img_flag(C3P_IMG_FLAG_BUFFER_LOCKED);  };

    inline ImgOrientation  orientation() {    return (ImgOrientation) (_imgflags & C3P_IMG_FLAG_ORIENTATION_MASK);  };
    inline bool            flipX() {          return _img_flag(C3P_IMG_FLAG_FLIP_X);         };
    inline bool            flipY() {          return _img_flag(C3P_IMG_FLAG_FLIP_Y);         };
    void  orientation(ImgOrientation);
    void  flipX(bool f);
    void  flipY(bool f);

    /* BEGIN ADAFRUIT GFX SPLICE */
    void drawFastHLine(PixUInt x, PixUInt y, PixUInt w, uint32_t color);
    void drawFastVLine(PixUInt x, PixUInt y, PixUInt h, uint32_t color);
    void drawLine(PixUInt x0, PixUInt y0, PixUInt x1, PixUInt y1, uint32_t color);
    void fillRect(PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color);
    void drawRect(PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color);
    void drawRoundRect(PixUInt x0, PixUInt y0, PixUInt w,  PixUInt h, PixUInt radius, uint32_t color);
    void fillRoundRect(PixUInt x0, PixUInt y0, PixUInt w,  PixUInt h,  PixUInt radius, uint32_t color);
    void drawTriangle(PixUInt x0,  PixUInt y0, PixUInt x1, PixUInt y1, PixUInt x2, PixUInt y2, uint32_t color);
    void fillTriangle(PixUInt x0,  PixUInt y0, PixUInt x1, PixUInt y1, PixUInt x2, PixUInt y2, uint32_t color);
    void drawCircle(PixUInt x0, PixUInt y0, PixUInt r, uint32_t color);
    void drawCircleHelper(PixUInt x0, PixUInt y0, PixUInt r, uint8_t quadrants, uint32_t color);
    void fillCircle(PixUInt x0, PixUInt y0, PixUInt r, uint32_t color);
    void fillCircleHelper(PixUInt x0, PixUInt y0, PixUInt r, uint8_t quadrants, PixUInt delta, uint32_t color);
    void fill(uint32_t color);
    void drawBitmap(PixUInt x, PixUInt y, const uint8_t* bitmap, PixUInt w, PixUInt h);

    void drawEllipse(PixUInt x0, PixUInt y0, PixUInt v_axis, PixUInt h_axis, float rotation, uint32_t color);
    //void drawArc(PixUInt x0, PixUInt y0, PixUInt v_axis, PixUInt h_axis, float rotation, float radians_start, float radians_stop, uint32_t color);

    void drawChar(PixUInt x, PixUInt y, unsigned char c, uint32_t color, uint32_t bg, uint8_t size);
    void writeChar(uint8_t c);
    void writeString(StringBuilder*);
    void writeString(const char*);
    void setCursor(PixUInt x, PixUInt y);
    void getTextBounds(const char *string, PixUInt x, PixUInt y, PixUInt* x1, PixUInt* y1, PixUInt* w, PixUInt* h);
    void getTextBounds(const uint8_t* s, PixUInt x, PixUInt y, PixUInt* x1, PixUInt* y1, PixUInt* w, PixUInt* h);
    void getTextBounds(StringBuilder*, PixUInt x, PixUInt y, PixUInt* x1, PixUInt* y1, PixUInt* w, PixUInt* h);
    void setTextSize(uint8_t s);
    void setTextColor(uint32_t c, uint32_t bg);
    void setTextColor(uint32_t c);
    PixUInt getFontWidth();
    PixUInt getFontHeight();
    inline void     setFont(const GFXfont* f) {   _gfxFont = (GFXfont*) f;    };
    inline GFXfont* getFont() {                   return _gfxFont;            };
    inline PixUInt  getCursorX() const {          return _cursor_x;           };
    inline PixUInt  getCursorY() const {          return _cursor_y;           };

    inline void textWrap(bool x) {  _img_set_flag(C3P_IMG_FLAG_TXT_WRAPS, x);         };
    inline bool textWrap() {        return _img_flag(C3P_IMG_FLAG_TXT_WRAPS);         };
    inline void cp437(bool x) {     _img_set_flag(C3P_IMG_FLAG_IS_STRICT_CP437, x);   };
    inline bool cp437() {           return _img_flag(C3P_IMG_FLAG_IS_STRICT_CP437);   };
    /* END ADAFRUIT GFX SPLICE */
    inline void textWrapsToZero(bool x) {  _img_set_flag(C3P_IMG_FLAG_TXT_WRAPS_TO_0, x);         };
    inline bool textWrapsToZero() {        return _img_flag(C3P_IMG_FLAG_TXT_WRAPS_TO_0);         };
    inline bool endianFlip() {              return _img_flag(C3P_IMG_FLAG_ENDIAN_FLIP);         };
    void bigEndian(bool x);

    static const char* const formatString(const ImgBufferFormat);
    static const float rotationAngle(const ImgOrientation);


  protected:
    uint8_t*        _buffer   = nullptr;
    PixUInt         _x        = 0;
    PixUInt         _y        = 0;

    inline uint8_t _bits_per_pixel() {     return _bits_per_pixel(_buf_fmt);    };
    inline bool _is_dirty() {              return _img_flag(C3P_IMG_FLAG_IS_FB_DIRTY);     };
    inline void _is_dirty(bool x) {        _img_set_flag(C3P_IMG_FLAG_IS_FB_DIRTY, x);     };
    inline void _is_framebuffer(bool x) {  _img_set_flag(C3P_IMG_FLAG_IS_FRAMEBUFFER, x);  };
    inline void _lock(bool x) {            _img_set_flag(C3P_IMG_FLAG_BUFFER_LOCKED, x);   };


  private:
    uint16_t        _imgflags    = 0;
    ImgBufferFormat _buf_fmt  = ImgBufferFormat::UNALLOCATED;  // TODO: Merge this into the flags.
    uint8_t         _textsize    = 0;       // Desired magnification of text to print()
    GFXfont*        _gfxFont     = nullptr; // Pointer to font
    PixUInt         _cursor_x    = 0;       // x location to start print()ing text
    PixUInt         _cursor_y    = 0;       // y location to start print()ing text
    uint32_t        _textcolor   = 0;       // Text color
    uint32_t        _textbgcolor = 0;       // Background color for text

    void _char_bounds(char c, PixUInt* x, PixUInt* y, PixUInt* minx, PixUInt* miny, PixUInt* maxx, PixUInt* maxy);
    int8_t _buffer_allocator();
    void _remap_for_orientation(PixUInt* x, PixUInt* y);

    /* Linearizes the X/y value in preparation for array indexing. */
    inline uint32_t _pixel_number(uint32_t x, uint32_t y) {      return ((y * _x) + x);    };

    /* Returns the byte offset in the buffer that holds the pixel. */
    inline uint32_t _pixel_offset(PixUInt x, PixUInt y) {    return ((_pixel_number(x, y) * _bits_per_pixel()) >> 3);  };

    inline bool  _is_ours() {     return _img_flag(C3P_IMG_FLAG_BUFFER_OURS); };
    inline void  _ours(bool l) {  _img_set_flag(C3P_IMG_FLAG_BUFFER_OURS, l); };

    inline uint16_t _img_flags() {               return _imgflags;            };
    inline bool _img_flag(uint16_t _flag) {       return (_imgflags & _flag);  };
    inline void _img_flip_flag(uint16_t _flag) {  _imgflags ^= _flag;          };
    inline void _img_clear_flag(uint16_t _flag) { _imgflags &= ~_flag;         };
    inline void _img_set_flag(uint16_t _flag) {   _imgflags |= _flag;          };
    inline void _img_set_flag(uint16_t _flag, bool nu) {
      if (nu) _imgflags |= _flag;
      else    _imgflags &= ~_flag;
    };

    static const uint8_t _bits_per_pixel(const ImgBufferFormat);
};

#endif   // __C3P_TYPE_IMG_H
