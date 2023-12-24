/*
File:   BlobPlotter.h
Author: J. Ian Lindsay
Date:   2023.12.17

These classes are built on top of the GfxUI classes, and implement various
  renderings of raw binary data.
*/

#ifndef __C3P_IMG_BLOB_PLOTTER_H
#define __C3P_IMG_BLOB_PLOTTER_H

#include "../Image.h"
#include "../../PriorityQueue.h"

class C3PValue;  // For-dec this so we don't have to bring that whole branch in.


/*******************************************************************************
* Styling
*******************************************************************************/
/*
* A pure-virtual class to assign colors to specific bytes.
*/
class BlobStyler {
  public:
    inline bool renderByteFrames() {          return _render_frames;   };
    inline void renderByteFrames(bool x) {    _render_frames = x;      };

    virtual int8_t   init(const uint8_t* PTR, const uint32_t LEN) =0;
    virtual uint32_t getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET) =0;


  protected:
    ImgBufferFormat _color_fmt;
    bool _render_frames;

    BlobStyler(Image* target) : _color_fmt(target->format()), _render_frames(false) {};
    ~BlobStyler() {};
};


/*
* A BlobStyler that generates a map of entropy within the dataset.
*/
class BlobStylerEntropyMap : public BlobStyler {
  public:
    BlobStylerEntropyMap(Image* target) : BlobStyler(target) {};
    ~BlobStylerEntropyMap() {};

    int8_t   init(const uint8_t* PTR, const uint32_t LEN);
    uint32_t getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET);


  protected:
    double  _stdev;
};


/*
* A BlobStyler to make a value heat map from a byte array.
*/
class BlobStylerHeatMap : public BlobStyler {
  public:
    BlobStylerHeatMap(Image* target, uint32_t color_base, uint32_t color_tween) :
        BlobStyler(target), _color_base(color_base), _color_tween(color_tween) {};
    ~BlobStylerHeatMap() {};

    int8_t   init(const uint8_t* PTR, const uint32_t LEN);
    uint32_t getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET);


  protected:
    uint32_t   _color_base;
    uint32_t   _color_tween;
};


/*
* A BlobStyler that allows for color-coding by explicit offsets.
*/
class BlobStylerExplicitFencing : public BlobStyler {
  public:
    BlobStylerExplicitFencing(Image* target) : BlobStyler(target) {};
    ~BlobStylerExplicitFencing() {};

    int8_t   init(const uint8_t* PTR, const uint32_t LEN);
    uint32_t getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET);

    /* Wipes all the existing fences. */
    inline void wipe() {  _fences.clear();  };
    int8_t addOffset(uint32_t offset, uint32_t color);


  protected:
    PriorityQueue<uint32_t> _fences;  // Priority holds offset, value is color.
};


/*******************************************************************************
* Geometries
*******************************************************************************/

/*
* A pure-virtual class to render a (possibly large) binary field into an
*   understandable representation.
*/
class BlobPlotter {
  public:
    BlobPlotter(
      BlobStyler* styler,
      C3PValue* src_blob, Image* target,
      uint32_t x = 0, uint32_t y = 0, uint32_t w = 0, uint32_t h = 0
    );
    ~BlobPlotter() {};

    void setParameters(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    inline void setBlob(C3PValue* blob) {     _src_blob = blob;  };
    inline uint32_t renderLength() {          return (_offset_stop - _offset_start);   };
    int8_t apply();


  protected:
    BlobStyler* _styler;
    C3PValue*   _src_blob;
    Image*      _target;
    uint32_t    _t_x;
    uint32_t    _t_y;
    uint32_t    _t_w;
    uint32_t    _t_h;
    uint32_t    _offset_start;
    uint32_t    _offset_stop;
    uint16_t    _val_trace;
    uint16_t    _bytes_wide;
    uint16_t    _bytes_high;
    bool        _force_render;   // Set when a frustum is changed.

    bool _needs_render();
    bool _able_to_render();
    int8_t _calculate_square_size(const uint32_t LEN, const uint32_t T_SIZE, uint32_t* square_size);
    inline uint32_t _pixels_available() {     return (_t_w * _t_h);   };
    virtual int8_t _curve_render(const uint8_t* PTR, const uint32_t OFFSET, const uint32_t LEN) =0;
};


/*
* A BlobPlotter for a HilbertCurve.
*/
class BlobPlotterHilbertCurve : public BlobPlotter {
  public:
    BlobPlotterHilbertCurve(
      BlobStyler* styler,
      C3PValue* src_blob, Image* target,
      uint32_t x = 0, uint32_t y = 0, uint32_t w = 0, uint32_t h = 0
    ) : BlobPlotter(styler, src_blob, target, x, y, w, h) {};
    ~BlobPlotterHilbertCurve() {};


  protected:
    virtual int8_t _curve_render(const uint8_t* PTR, const uint32_t OFFSET, const uint32_t LEN);
    uint32_t _bin_to_reflected_gray(uint32_t idx);
    uint32_t _reflected_gray_to_idx(uint32_t gray);
};


/*
* A BlobPlotter for a PeanoCurve.
*/
class BlobPlotterLinear : public BlobPlotter {
  public:
    BlobPlotterLinear(
      BlobStyler* styler,
      C3PValue* src_blob, Image* target,
      uint32_t x = 0, uint32_t y = 0, uint32_t w = 0, uint32_t h = 0
    ) : BlobPlotter(styler, src_blob, target, x, y, w, h) {};
    ~BlobPlotterLinear() {};


  protected:
    virtual int8_t _curve_render(const uint8_t* PTR, const uint32_t OFFSET, const uint32_t LEN);
};

#endif  // __C3P_IMG_BLOB_PLOTTER_H
