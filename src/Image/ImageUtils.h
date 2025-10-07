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
#include "../Quaternion.h"


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



/*
* This is a generating class that generates Perlin noise at a given region
*   of the given Image.
*/
class ImgPerlinNoise : public PerlinNoise {
  public:
    /**
     * @param target    Pointer to the target Image.
     * @param x         X coordinate of the top-left corner of the region.
     * @param y         Y coordinate of the top-left corner of the region.
     * @param width     Width of the region.
     * @param height    Height of the region.
     * @param scale     The "zoom" of the noise (higher = more zoomed-out).
     * @param octaves   Number of octaves to sum (controls detail).
     * @param persistence  Amplitude falloff per octave.
     */
    ImgPerlinNoise(Image* target     = nullptr,
                PixUInt x         = 0,
                PixUInt y         = 0,
                PixUInt width     = 0,
                PixUInt height    = 0,
                float scale       = 1.0f,
                int octaves       = 1,
                float persistence = 0.5f);
    ~ImgPerlinNoise() {};

    /**
    * Applies Perlin noise into the target region.
    * @return 0 on success, -1 if the target image pointer is null.
    */
    int8_t apply();
    inline void blendMode(const BlendMode E) {  _blend_mode = E;  };


  private:
    Image*  _target;
    PixUInt _t_x;
    PixUInt _t_y;
    PixUInt _t_w;
    PixUInt _t_h;
    BlendMode _blend_mode;
};



/*
* This is a transform class that applies artificial NTSC distortions to
*   an image. Ironically, this is purely for aesthetics.
*/
class GfxNTSCEffect {
  public:
    GfxNTSCEffect(Image* in, Image* out);
    //GfxNTSCEffect(ImageSubframe in_frame, ImageSubframe out_frame);
    ~GfxNTSCEffect() {};

    int8_t setSourceFrame(const PixAddr A, const PixUInt W, const PixUInt H);
    int8_t apply();

    // As a percentage.
    inline void noiseFactor(const float NOISE) {  _noise_level = NOISE;  };


  private:
    //ImageSubframe _source;
    //ImageSubframe _target;
    Image* _source;
    Image* _target;
    PixAddr _src_addr;
    PixUInt _width;
    PixUInt _height;
    float   _noise_level;

    uint8_t _clip_to_byte(int32_t);
};


/*
 * Applies a CRT-style bloom and edge curvature effect to an image region.
 * Bloom: a simple weighted blur according to bloom factor.
 * Edge curvature: darkens or brightens toward edges based on curvature factor.
 */
class GfxCRTBloomEffect {
 public:
  // Constructs with source and target images
  GfxCRTBloomEffect(Image* in, Image* out);
  ~GfxCRTBloomEffect() {};

  // Set region of operation
  int8_t setSourceFrame(const PixAddr A, const PixUInt W, const PixUInt H);

  // Apply effect; returns 0 on success, -1 on error
  int8_t apply();

  // Bloom intensity [0..1]
  void bloomFactor(const float FACTOR) { _bloom_factor = FACTOR; }

  // Edge curvature [0..1]
  void edgeCurvature(const float CURV) { _edge_curvature = CURV; }


 private:
  Image*   _source;
  Image*   _target;
  PixAddr  _src_addr;
  PixUInt  _width;
  PixUInt  _height;
  float    _bloom_factor;
  float    _edge_curvature;

  uint8_t _clip_to_byte(int32_t v);
};


/*******************************************************************************
* Projection rendering of some 3-space objects.
*******************************************************************************/
// Work structure to hold projected point and depth.
struct PointZ {
  int x;
  int y;
  float z;
};

/*
* Class for rendering a pretty 3-vector.
* Render will auto-scale to the vector's size, preserving aspect ratio across axes.
* The render should be distance shaded to make perspective clearer.
* X-axis is left(-)/right(+).
* Y-axis is out-of(-)/into(+) the screen.
* Z-axis is down(-)/up(+).
* Default colors for axes is to match blender conventions.
* If drawValue() is enabled, the class will use the text functions in the Image class to
*   print the value in the form "<x, y, z>" with the proper colors for each component.
*/
class Vector3Render {
  public:
    Vector3Render(Image* i);
    ~Vector3Render() {};

    // Renders the globe grid
    void render(bool force = false);

    // Set the region of the image to render into.
    int8_t setSourceFrame(const PixAddr A, const PixUInt W, const PixUInt H);
    int8_t setVector(float x, float y, float z);

    // Style settings...
    void setLatLonDivisions(const uint8_t LAT_DIVS, const uint8_t LON_DIVS);
    void setColors(
      const uint32_t COLOR_X,
      const uint32_t COLOR_Y,
      const uint32_t COLOR_Z,
      const uint32_t COLOR_VECTOR,
      const uint32_t COLOR_BG
    );

    // Setting a value of zero will disable grid marks for that axis.
    void setgridMarks(
      const uint8_t MARKS_X,
      const uint8_t MARKS_Y,
      const uint8_t MARKS_Z
    );

    // Orientation as measured as a deviation from the X-Z plane.
    void setOrientation(const float PITCH, const float ROLL);
    void setOrientation(const Quaternion);

    inline bool needRerender() {   return _need_rerender;  };

    inline bool drawAnchorLines() {      return _draw_anchor_lines;  };
    inline void drawAnchorLines(const bool enabled) { _draw_anchor_lines = enabled; _need_rerender = true; }
    inline bool drawValue() {      return _draw_text_value;  };
    inline void drawValue(const bool enabled) { _draw_text_value = enabled; _need_rerender = true; }


  protected:
    Image*   _img;
    PixAddr  _addr;   // The pixel in the target image that is our upper-left corner.
    PixUInt  _width;  // The width of this frame.
    PixUInt  _height; // The height of this frame.
    uint32_t _vector_color;
    uint32_t _axis_color_x;
    uint32_t _axis_color_y;
    uint32_t _axis_color_z;
    uint32_t _background_color;
    uint8_t  _x_grid_marks;  // How many value tick marks to place on each axis?
    uint8_t  _y_grid_marks;  // How many value tick marks to place on each axis?
    uint8_t  _z_grid_marks;  // How many value tick marks to place on each axis?
    bool     _need_rerender;
    bool     _draw_anchor_lines;  // Draw axis lines back to their normal planes from the vector tip to clearly indicate placement following projection?
    bool     _draw_text_value;    // Print the value in text at the vector's tip?

  private:
    float _vec_x;
    float _vec_y;
    float _vec_z;
    float _pitch;
    float _roll;
    float _sin_pitch;  // Cached to avoid costly re-work.
    float _cos_pitch;  // TODO: Might rework internally as a quaterion.
    float _sin_roll;
    float _cos_roll;

    void _draw_axes();
    void _draw_vector();
    void _project_point(float x0, float y0, float z0, PointZ &out);
};



/*
* Class for rendering a shaded wire-frame globe. Provides pixel-to-LAT/LON mapping.
*/
class GlobeRender {
  public:
    GlobeRender(Image* i);
    ~GlobeRender() {};

    // Renders the globe grid
    void render(bool force = false);

    // Renders the globe grid and plots a marker at the given latitude/longitude (radians)
    void renderWithMarker(float latitude, float longitude);

    // Converts a pixel on the rendered globe back to latitude/longitude (radians)
    // @returns true if the pixel hits the visible hemisphere, false otherwise
    bool pixelToLatLon(const PixAddr ADDR, float &latitude, float &longitude);

    // Set the region of the image to render into.
    int8_t setSourceFrame(const PixAddr A, const PixUInt W, const PixUInt H);

    // Style settings...
    void setLatLonDivisions(const uint8_t LAT_DIVS, const uint8_t LON_DIVS);
    void setColors(const uint32_t COLOR, const uint32_t BG_COLOR);
    void setOrientation(const float PITCH, const float ROLL);
    void setOrientation(const Quaternion);

    inline bool needRerender() {   return _need_rerender;  };


  protected:
    Image*   _img;
    PixAddr  _addr;   // The pixel in the target image that is our upper-left corner.
    PixAddr  _center;
    PixUInt  _width;  // The width of this frame.
    PixUInt  _height; // The height of this frame.
    PixUInt  _radius;
    uint32_t _sphere_color;
    uint32_t _background_color;
    uint8_t  _lat_lines;
    uint8_t  _lon_lines;
    uint8_t  _curve_segments;  // TODO: Scale intelligently by _width/_height.
    bool     _need_rerender;

  private:
    float _pitch;
    float _roll;
    float _sin_pitch;  // Cached to avoid costly re-work.
    float _cos_pitch;  // TODO: Might rework internally as a quaterion.
    float _sin_roll;
    float _cos_roll;
};



/*******************************************************************************
* TODO: Stub classes for later....
*/

// Phong algorithm for cell-shading.

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
* This is a generating class that takes an array representing a heat-map,
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
