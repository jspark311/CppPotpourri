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
class BlobPlotter;

/*******************************************************************************
* Enums
*******************************************************************************/
enum class BlobPlotterID : uint8_t {
  NONE = 0,
  LINEAR,
  HILBERT,
  INVALID
};


enum class BlobStylerID : uint8_t {
  NONE = 0,
  HEAT,
  ENTROPY,
  FENCING,
  INVALID
};



/*******************************************************************************
* Styling
*******************************************************************************/
/*
* A pure-virtual class to assign colors to specific bytes.
* TODO: Style class should do the rendering. Not simply provide the styles.
*/
class BlobStyler {
  public:
    virtual ~BlobStyler() {};

    inline const BlobStylerID stylerID() {    return _STYLER_ID;       };
    inline bool renderByteFrames() {          return _render_frames;   };
    inline void renderByteFrames(bool x) {    _render_frames = x;      };

    virtual int8_t   init(const uint8_t* PTR, const uint32_t LEN) =0;
    virtual uint32_t getColor(const uint8_t* PTR, const uint32_t LEN, const uint32_t OFFSET) =0;


  protected:
    friend class BlobPlotter;
    const BlobStylerID _STYLER_ID;
    ImgBufferFormat _color_fmt;
    bool _render_frames;

    BlobStyler(const BlobStylerID S_ID, Image* target) :
      _STYLER_ID(S_ID), _color_fmt(target->format()), _render_frames(false) {};
};


/*
* A BlobStyler that generates a map of entropy within the dataset.
*/
class BlobStylerEntropyMap : public BlobStyler {
  public:
    BlobStylerEntropyMap(Image* target) : BlobStyler(BlobStylerID::ENTROPY, target) {};
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
        BlobStyler(BlobStylerID::HEAT, target), _color_base(color_base), _color_tween(color_tween) {};
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
    BlobStylerExplicitFencing(Image* target) : BlobStyler(BlobStylerID::FENCING, target) {};
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
      const BlobPlotterID P_ID, BlobStyler* styler,
      C3PValue* src_blob, Image* target,
      PixUInt x = 0, PixUInt y = 0, PixUInt w = 0, PixUInt h = 0
    );
    virtual ~BlobPlotter() {};

    void setParameters(PixUInt x, PixUInt y, PixUInt w, PixUInt h);
    inline void setBlob(C3PValue* blob) {     _src_blob = blob;  };

    inline const BlobPlotterID plotterID() {  return _PLOTTER_ID;   };
    inline uint32_t renderLength() {          return (_offset_stop - _offset_start);   };
    inline uint32_t bytesWide() {             return _bytes_wide;   };
    inline uint32_t bytesHigh() {             return _bytes_high;   };

    int8_t apply(bool force = false);

    inline void setStyler(BlobStyler* styler) {     _styler = styler;  };

    // This is an optional feature that allows for a Cartesian mapping of the
    //   curve back into an offset.
    // TODO: This costs extravegantly of memory, but saves on CPU. Think this
    //   through a bit better... The only other direct alternative is
    //   back-clocking the black magic in the Hilbert curve. And although
    //   possible, doing it will require sustained careful thought.
    //   Would be cleaner to use a callback fxn, probably... But more overhead.
    inline void setMapMem(uint16_t* PTR, const uint32_t LEN) {  _mapping_ptr = PTR;  _mapping_len = LEN;  };



  protected:
    const BlobPlotterID _PLOTTER_ID;
    bool        _force_render;   // Set when a frustum is changed.
    BlobStyler* _styler;    // TODO: Remove?
    C3PValue*   _src_blob;
    Image*      _target;
    PixUInt     _t_x;
    PixUInt     _t_y;
    PixUInt     _t_w;
    PixUInt     _t_h;
    uint32_t    _offset_start;
    uint32_t    _offset_stop;
    uint16_t    _val_trace;
    uint16_t    _bytes_wide;
    uint16_t    _bytes_high;
    PixUInt     _square_size;
    uint16_t*   _mapping_ptr;
    uint32_t    _mapping_len;

    bool _needs_render();
    bool _able_to_render();
    int8_t _calculate_square_size(const uint32_t LEN, const uint32_t T_SIZE);
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
      PixUInt x = 0, PixUInt y = 0, PixUInt w = 0, PixUInt h = 0
    ) : BlobPlotter(BlobPlotterID::HILBERT, styler, src_blob, target, x, y, w, h) {};
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
      PixUInt x = 0, PixUInt y = 0, PixUInt w = 0, PixUInt h = 0
    ) : BlobPlotter(BlobPlotterID::LINEAR, styler, src_blob, target, x, y, w, h) {};
    ~BlobPlotterLinear() {};


  protected:
    virtual int8_t _curve_render(const uint8_t* PTR, const uint32_t OFFSET, const uint32_t LEN);
};

#endif  // __C3P_IMG_BLOB_PLOTTER_H
