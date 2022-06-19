/*
File:   ImageUtils.h
Author: J. Ian Lindsay
Date:   2022.05.27

This source file was never part of Adafruit's library. They are small graphics
  utilities that help implement simple UIs.
*/

#include "Image.h"
#include "../SensorFilter.h"
#include "../TripleAxisPipe/TripleAxisCompass.h"


#ifndef __MANUVR_IMG_UTILS_H
#define __MANUVR_IMG_UTILS_H

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

    void drawGraph(
      int x, int y, int w, int h, uint color0, uint color1, uint color2,
      bool draw_base, bool draw_v_ticks, bool draw_h_ticks,
      SensorFilter<float>* filt0, SensorFilter<float>* filt1, SensorFilter<float>* filt2
    );

    void drawGraph(
      int x, int y, int w, int h, uint color0, uint color1,
      bool draw_base, bool draw_v_ticks, bool draw_h_ticks,
      SensorFilter<float>* filt0, SensorFilter<float>* filt1
    );

    void drawGraph(
      int x, int y, int w, int h, uint color,
      bool draw_base, bool draw_v_ticks, bool draw_h_ticks,
      SensorFilter<float>* filt
    );

    void drawGraph(
      int x, int y, int w, int h, uint color,
      bool draw_base, bool draw_v_ticks, bool draw_h_ticks,
      SensorFilter<uint32_t>* filt
    );


    void drawProgressBarH(
      int x, int y, int w, int h, uint color,
      bool draw_base, bool draw_val, float percent
    );

    void drawProgressBarV(
      int x, int y, int w, int h, uint color,
      bool draw_base, bool draw_val, float percent
    );

    void drawCompass(
      int x, int y, int w, int h,
      bool scale_needle, bool draw_val, float bearing_field, float bearing_true_north
    );

    void drawHeatMap(
      uint x, uint y, uint w, uint h,
      SensorFilter<float>* filt,
      uint32_t flags,
      float range_lock_low = 0.0f, float range_lock_hi = 0.0f
    );

    void drawVector(
      int x, int y, int w, int h, uint color,
      bool draw_axes, bool draw_val, float vx, float vy, float vz
    );

    void drawSphere(
      int x, int y, int w, int h,
      bool opaque,
      int meridians, int parallels,
      float euler_about_x, float euler_about_y   // TODO: A quat would be cleaner.
    );

    void draw_data_view_selector(
      int x, int y, int w, int h,
      DataVis opt0, DataVis opt1, DataVis opt2, DataVis opt3, DataVis opt4, DataVis opt5,
      DataVis selected
    );

    void drawButton(int x, int y, int w, int h, uint color, bool pressed);
    void drawScrollbarH(int x, int y, int w, int h, uint color, float pos);
    void drawScrollbarV(int x, int y, int w, int h, uint color, float pos);


  private:
    Image* _img;
    //FlagContainer32  _flags;

    void _draw_graph_frame(int* x, int* y, int* w, int* h, uint color, uint32_t flags);
    void _draw_graph_text_overlay(int x, int y, int w, int h, uint color, uint32_t flags, float v_max, float v_min, float v_scale, float last_datum);
    void _draw_graph_dataset(int x, int y, int w, int h, uint color, uint32_t flags, float* dataset, uint32_t data_len);

    void _apply_color_map();
};



/*
* This is a data processing class that applies artificial NTSC distortions to
*   an image. Ironically, this is purely for aesthetics.
*/
class GfxNTSCEffect {
  public:
    GfxNTSCEffect(Image* i_s, Image* i_t) : _source(i_s), _target(i_t) {};
    ~GfxNTSCEffect() {};

    int8_t apply();


  private:
    Image* _source;
    Image* _target;
};


/*
* This is a data processing class that scales the source Image, and writes it
*   into the target.
* Can also be used to do a region-bounded copy from one image to another.
*/
class ImageScaler {
  public:
    ImageScaler(
      Image* i_s, Image* i_t, float scale,
      int s_x = 0, int s_y = 0, int s_w = 0, int s_h = 0,
      int t_x = 0, int t_y = 0
    );
    ~ImageScaler() {};

    int8_t apply();
    inline float scale() {         return _scale;    };
    inline void scale(float x) {   _scale = x;       };

    void setParameters(float scale, int s_x, int s_y, int s_w, int s_h, int t_x, int t_y);


  private:
    Image* _source;
    Image* _target;
    float _scale;
    int   _s_x;
    int   _s_y;
    int   _s_w;
    int   _s_h;
    int   _t_x;
    int   _t_y;
};


#endif  // __MANUVR_IMG_UTILS_H