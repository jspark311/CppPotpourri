/*
File:   ImageUtils.h
Author: J. Ian Lindsay
Date:   2022.05.27

These classes are tools built on top of the Image class. This source file was
  never part of Adafruit's library. They are small graphics utilities that help
  implement simple UIs.

NOTE: This header file should evolve into little more than a collection
  of #includes, and possibly a small number of shared types.

TODOING: UIGfxWrapper is somewhat vestigial. It should be subsumed (or divided)
  into other classes.
*/

#ifndef __C3P_IMG_UTILS_H
#define __C3P_IMG_UTILS_H

#include "Image.h"
#include "../TimeSeries/TimeSeries.h"
#include "../Identity/Identity.h"
#include "../M2MLink/M2MLink.h"
#include "ImageUtils/ImageGraph.h"


/*******************************************************************************
* UIGfxWrapper flags
*******************************************************************************/
#define GFXUI_FLAG_LOCK_RANGE_V             0x00800000   // Lock the V range.
#define GFXUI_FLAG_TEXT_RANGE_V             0x01000000   // Text overlay for axis values.
#define GFXUI_FLAG_TEXT_VALUE               0x02000000   // Text overlay for current value.
#define GFXUI_FLAG_PARTIAL_REDRAW           0x04000000   // Partial redraw
#define GFXUI_FLAG_FULL_REDRAW              0x08000000   // Full redraw
#define GFXUI_FLAG_DRAW_RULE_H              0x10000000   //
#define GFXUI_FLAG_DRAW_RULE_V              0x20000000   //
#define GFXUI_FLAG_DRAW_TICKS_H             0x40000000   //
#define GFXUI_FLAG_DRAW_TICKS_V             0x80000000   //


enum class DataVis : uint8_t {
  NONE          = 0,  // A time-series graph.
  GRAPH         = 1,  // A time-series graph.
  VECTOR        = 2,  // A projected 3-space vector.
  COMPASS       = 3,  // A compass render.
  FIELD         = 4,  // A 2d array.
  TEXT          = 5   // Prefer alphanumeric readout.
};


const char* const getDataVisString(const DataVis);

/*
* The functions provided here are designed to ease implementation of UI on
*   displays in the 10-kilopixel regime.
*/
class UIGfxWrapper {
  public:
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t active_color;
    uint32_t inactive_color;

    UIGfxWrapper(Image*);
    ~UIGfxWrapper() {};

    inline Image* img() {   return _img;   };

    void drawProgressBarH(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color,
      bool draw_base, bool draw_val, float percent
    );

    void drawProgressBarV(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color,
      bool draw_base, bool draw_val, float percent
    );

    void drawZoomBarH(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color,
      bool draw_val, float fraction_left, float fraction_right
    );

    void drawZoomBarV(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color,
      bool draw_val, float fraction_top, float fraction_bot
    );

    void drawCompass(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h,
      bool scale_needle, bool draw_val, float bearing_field, float bearing_true_north
    );

    void drawHeatMap(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h,
      TimeSeries<float>* filt,
      uint32_t flags,
      float range_lock_low = 0.0f, float range_lock_hi = 0.0f
    );

    void drawVector(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t color,
      bool draw_axes, bool draw_val, float vx, float vy, float vz
    );

    void drawSphere(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h,
      bool opaque,
      int meridians, int parallels,
      float euler_about_x, float euler_about_y   // TODO: A quat would be cleaner.
    );

    void draw_data_view_selector(
      PixUInt x, PixUInt y, PixUInt w, PixUInt h,
      DataVis opt0, DataVis opt1, DataVis opt2, DataVis opt3, DataVis opt4, DataVis opt5,
      DataVis selected
    );


  private:
    Image* _img;

    void _apply_color_map();
};



/*
* This is a data processing class that scales the source Image, and writes it
*   into the target.
* Can also be used to do a region-bounded copy from one image to another (with
*   or without scaling).
*
* Scaling constraints:
* 1) Over-unity scaling ("zooming in") must be done in round-integer pixel ratios. 1x, 2x, 3x...
* 2) Under-unity scaling ("zooming out") must be done in round-integer pixel ratios. 1/2x, 1/3x, 1/4x...
* The class will handle both of the constraints, but it will do so by allowing any rounding truncations
*   to manifest as "jitter" in W and H written to the target image.
*/
class ImageScaler {
  public:
    ImageScaler(
      Image* i_s, Image* i_t, float scale,
      PixUInt s_x = 0, PixUInt s_y = 0, PixUInt s_w = 0, PixUInt s_h = 0,
      PixUInt t_x = 0, PixUInt t_y = 0
    );
    ~ImageScaler() {};

    int8_t apply();
    inline float scale() {         return _scale;    };
    inline void scale(float x) {   _scale = x;       };

    void setParameters(float scale, PixUInt s_x, PixUInt s_y, PixUInt s_w, PixUInt s_h, PixUInt t_x, PixUInt t_y);


  private:
    Image*  _source;
    Image*  _target;
    float   _scale;
    PixUInt _s_x;
    PixUInt _s_y;
    PixUInt _s_w;
    PixUInt _s_h;
    PixUInt _t_x;
    PixUInt _t_y;
};



/*
* This is an adapter class that casts an Image over a Link.
*/
class ImageCaster {
  public:
    ImageCaster(M2MLink* l, Image* i_s, PixUInt x = 0, PixUInt y = 0, PixUInt w = 0, PixUInt h = 0);
    ~ImageCaster() {};

    //int M2MLink::send(KeyValuePair* kvp, bool need_reply);
    int8_t apply();
    bool   busy();


  private:
    const uint32_t _id;
    M2MLink*       _link;
    Image*         _source;
    PixUInt        _s_x;
    PixUInt        _s_y;
    PixUInt        _s_w;
    PixUInt        _s_h;
};


/*
* This is an adapter class that writes an Image from data received over a Link.
*/
class ImageCatcher {
  public:
    ImageCatcher();
    ImageCatcher(Image* i_t, PixUInt x, PixUInt y, PixUInt w = 0, PixUInt h = 0);
    ~ImageCatcher();

    int8_t apply(KeyValuePair* kvp);   // Takes a serialized Image.
    inline Image* img() {         return _target;   };
    inline bool   allocated() {   return ((nullptr != _target) && (_target->allocated()));   };


  private:
    uint32_t _id;
    Image*   _target;
    PixUInt  _t_x;
    PixUInt  _t_y;
    PixUInt  _t_w_max;
    PixUInt  _t_h_max;
    bool     _target_is_ours;
};




/*******************************************************************************
* TODO: Stub classes for later....
*/

/*
* This is a transform class that blends two Images and writes the result into a
*   third.
*/
class ImageCrossfader {
  public:
    ImageCrossfader(Image* i_0, Image* i_1, Image* i_t);
    ~ImageCrossfader() {};

    int8_t apply();
    int8_t blendAlgo();

    inline void  sourceBias0(float bias) {     _s0_bias = bias;   };
    inline void  sourceBias1(float bias) {     _s1_bias = bias;   };


  private:
    Image*   _source0;
    Image*   _source1;
    Image*   _target;
    float    _s0_bias;
    float    _s1_bias;
    PixUInt  _s_x;
    PixUInt  _s_y;
    PixUInt  _s_w;
    PixUInt  _s_h;
    PixUInt  _t_x;
    PixUInt  _t_y;
    uint8_t  _algo;
};


/*
* This is a transform class that applies artificial NTSC distortions to
*   an image. Ironically, this is purely for aesthetics.
*/
class GfxNTSCEffect {
  public:
    GfxNTSCEffect(Image* i_s, Image* i_t);
    ~GfxNTSCEffect() {};

    int8_t apply();


  private:
    Image* _source;
    Image* _target;
};


/*
* This is a transform class that generates an authentication code for the source Image,
*   and then steganographically embeds it into the Image itself, along with an
*   optional payload. That is, it modifies the source Image. For theory and
*   operation, see BuriedUnderTheNoiseFloor.
*/
class ImageSigner {
  public:
    ImageSigner(
      Image* i_s, Identity* signing_ident, uint8_t* payload = nullptr, uint32_t payload_len = 0
    );
    ~ImageSigner() {};

    int8_t sign();
    int8_t signWithParameters();
    bool   busy();


  private:
    Image*    _source;
    Identity* _signing_ident;
    uint8_t*  _pl;
    uint32_t  _pl_len;
};


/*
* This is a class that tries to authenticate a given Image against a given
*   Identity, and extract any payloads that may be steganographically embedded
*   within it. It does not modify the source Image. For theory and
*   operation, see BuriedUnderTheNoiseFloor.
*/
class ImageAuthenticator {
  public:
    ImageAuthenticator(Image* i_s, Identity* verify_ident);
    ~ImageAuthenticator() {};

    int8_t verify();
    int8_t verifyWithParameters();
    bool   busy();
    bool   authenticated();
    bool   foundSig();

    inline uint8_t* payload() {         return  _pl;        };
    inline uint32_t payloadLength() {   return  _pl_len;    };


  private:
    Image*    _source;
    Identity* _verify_ident;
    uint8_t*  _pl;
    uint32_t  _pl_len;
};



/*
* This is a generating class that generates Perlin noise at a given region
*   of the given Image.
*/
class PerlinNoise {
  public:
    PerlinNoise(Image* i_t, PixUInt x, PixUInt y, PixUInt w, PixUInt h);
    ~PerlinNoise() {};

    int8_t apply();


  private:
    Image*  _target;
    PixUInt _t_x;
    PixUInt _t_y;
    PixUInt _t_w;
    PixUInt _t_h;
};


/*
* This is a generating class that takes an array representing a heat-map,
* Can also be used to do a region-bounded copy from one image to another.
*/
class ImageHeatMap {
  public:
    ImageHeatMap(Image* i_t, PixUInt x, PixUInt y, PixUInt w, PixUInt h);
    ~ImageHeatMap() {};

    //int8_t apply(SensorFilter*);


  private:
    Image*  _target;
    PixUInt _t_x;
    PixUInt _t_y;
    PixUInt _t_w;
    PixUInt _t_h;
};


#endif  // __C3P_IMG_UTILS_H
