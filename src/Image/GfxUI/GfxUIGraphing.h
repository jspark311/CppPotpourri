/*
File:   GfxUIGraphing.h
Author: J. Ian Lindsay
Date:   2023.04.07

NOTE: Do not include this file directly. Only via GfxUI.h.

These classes are built on top of the GfxUI classes, and implement data graphing
  elements of a UI.
*/

#ifndef __C3P_GFXUI_KIT_GRAPHING_H
#define __C3P_GFXUI_KIT_GRAPHING_H

#include "../Image.h"
#include "../ImageUtils.h"
#include "../ImageUtils.h"
#include "../../TimeSeries/TimeSeries.h"

//////////////////////////////////
// TODO: Stark Fist of Removal
#define GFXUI_SENFILT_FLAG_SHOW_VALUE         0x01000000   //
#define GFXUI_SENFILT_FLAG_SHOW_RANGE         0x02000000   //
#define GFXUI_SENFILT_FLAG_AUTOSCALE_X        0x04000000   //
#define GFXUI_SENFILT_FLAG_AUTOSCALE_Y        0x08000000   //
#define GFXUI_SENFILT_FLAG_DRAW_GRID          0x10000000   //
#define GFXUI_SENFILT_FLAG_DRAW_CURVE         0x20000000   //
#define GFXUI_SENFILT_FLAG_LOCK_GRID          0x40000000   //
#define GFXUI_SENFILT_FLAG_GRAPH_NONFULL_WIN  0x80000000   // If set, the render will be attempted even if the filter window is not full.
// TODO: /Stark Fist
//////////////////////////////////



/*******************************************************************************
* Graphical tools for manipulating filters.
*******************************************************************************/

/* A basic pane that shows an annotated graph of a given TimeSeries. */
template <class T> class GfxUITimeSeries : public GfxUIElement {
  public:
    ImageGraphTrace<T> trace_settings;

    GfxUITimeSeries(const GfxUILayout lay, const GfxUIStyle sty, TimeSeries<T>* sf, uint32_t f = 0) : GfxUIElement(lay, sty, f | GFXUI_FLAG_ALWAYS_REDRAW), _filter(sf) {};
    ~GfxUITimeSeries() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    inline void showValue(bool x) {        trace_settings.show_value   = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool showValue() {              return trace_settings.show_value;  };
    inline void showRangeX(bool x) {       trace_settings.show_x_range = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool showRangeX() {             return trace_settings.show_x_range;  };
    inline void showRangeY(bool x) {       trace_settings.show_y_range = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool showRangeY() {             return trace_settings.show_y_range;  };
    inline void graphAutoscaleX(bool x) {  trace_settings.autoscale_x  = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool graphAutoscaleX() {        return trace_settings.autoscale_x; };
    inline void graphAutoscaleY(bool x) {  trace_settings.autoscale_y  = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool graphAutoscaleY() {        return trace_settings.autoscale_y; };
    inline void drawCurve(bool x) {        trace_settings.draw_curve   = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool drawCurve() {              return trace_settings.draw_curve;  };
    inline void drawGrid(bool x) {         trace_settings.draw_grid    = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool drawGrid() {               return trace_settings.draw_grid;       };
    inline void lockGridX(bool x) {        trace_settings.grid_lock_x  = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool lockGridX() {              return trace_settings.grid_lock_x;     };
    inline void lockGridY(bool x) {        trace_settings.grid_lock_y  = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline bool lockGridY() {              return trace_settings.grid_lock_y;     };
    inline void majorDivX(PixUInt x) {     trace_settings.major_grid_x = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };
    inline void majorDivY(PixUInt x) {     trace_settings.major_grid_y = x;  _class_set_flag(GFXUI_FLAG_NEED_RERENDER);  };

    inline void firstIdxRendered(uint32_t x) {  _left_most_data_idx = x;    };
    inline bool firstIdxRendered() {            return _left_most_data_idx; };

    inline TimeSeries<T>* dataset() {    return _filter;  };



  private:
    TimeSeries<T>* _filter;
    uint32_t _left_most_data_idx = 0;

    bool _sync_settings();   // Called to copy trace_settings into the active config.
};


/* The basic pane with control elements for runtime behavior adjustment. */
template <class T> class GfxUIGraphWithCtrl : public GfxUIElement {
  public:
    GfxUIGraphWithCtrl(const GfxUILayout lay, const GfxUIStyle sty, TimeSeries<T>* sf, uint32_t f = 0);
    ~GfxUIGraphWithCtrl() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    inline GfxUITimeSeries<T>* graphRender() {  return &_graph;  };
    inline void showValue(bool x) {           _btn_show_value.pressed(x);     };
    inline void showRangeX(bool x) {          _btn_show_range_x.pressed(x);   };
    inline void showRangeY(bool x) {          _btn_show_range_y.pressed(x);   };
    inline void graphAutoscaleX(bool x) {     _btn_autoscale_x.pressed(x);    };
    inline void graphAutoscaleY(bool x) {     _btn_autoscale_y.pressed(x);    };
    inline void drawCurve(bool x) {           _btn_draw_curve.pressed(x);     };
    inline void drawGrid(bool x) {            _btn_draw_grid.pressed(x);      };
    inline void lockGridX(bool x) {           _btn_grid_lock_x.pressed(x);    };
    inline void lockGridY(bool x) {           _btn_grid_lock_y.pressed(x);    };
    inline void majorDivX(PixUInt x) {        _graph.majorDivX(x);            };
    inline void majorDivY(PixUInt x) {        _graph.majorDivY(x);            };

    inline GfxUIZoomSlider* getSlider() {  return &_slider_x_axis;  };   // TODO: This should be unnecessary.


  private:
    GfxUIGroup      _ctrl_group;
    GfxUITextButton _btn_autoscale_x;
    GfxUITextButton _btn_autoscale_y;
    GfxUITextButton _btn_show_range_x;
    GfxUITextButton _btn_show_range_y;
    GfxUITextButton _btn_draw_curve;
    GfxUITextButton _btn_show_value;
    GfxUITextButton _btn_draw_grid;
    GfxUITextButton _btn_grid_lock_x;
    GfxUITextButton _btn_grid_lock_y;
    GfxUIGroup      _major_x_group;
    GfxUIGroup      _major_y_group;
    GfxUIZoomSlider _slider_x_axis;
    GfxUITimeSeries<T> _graph;
    T           _y_axis_min;
    T           _y_axis_max;
};


#define GFXUI_SF_CTRL_OFFSET_PX   180

/* Constructor */
template <class T> GfxUIGraphWithCtrl<T>::GfxUIGraphWithCtrl(const GfxUILayout lay, const GfxUIStyle sty, TimeSeries<T>* sf, uint32_t f) :
  GfxUIElement(lay, sty, (f | GFXUI_FLAG_TRACK_POINTER | GFXUI_FLAG_ALWAYS_REDRAW)),
  _ctrl_group(
    internalPosX(), (internalPosY() + (internalHeight()-64)),  // Bottom-float pattern
    internalWidth(), 60
  ),

  _btn_autoscale_x(
    GfxUILayout(
      (_ctrl_group.elementPosX()+GFXUI_SF_CTRL_OFFSET_PX), _ctrl_group.elementPosY(),
      32, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "X"
  ),
  _btn_autoscale_y(
    GfxUILayout(
      (_btn_autoscale_x.elementPosX() + _btn_autoscale_x.elementWidth()), _btn_autoscale_x.elementPosY(),
      32, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Y"
  ),
  _btn_show_range_x(
    GfxUILayout(
      (_ctrl_group.elementPosX()+GFXUI_SF_CTRL_OFFSET_PX), (_ctrl_group.elementPosY() + (_ctrl_group.elementHeight() >> 1)),
      32, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "X"
  ),
  _btn_show_range_y(
    GfxUILayout(
      (_btn_show_range_x.elementPosX() + _btn_show_range_x.elementWidth()), _btn_show_range_x.elementPosY(),
      32, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Y"
  ),


  _btn_draw_curve(
    GfxUILayout(
      (_btn_show_range_y.elementPosX()+_btn_show_range_y.elementWidth()+48), _ctrl_group.elementPosY(),
      128, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Draw curve"
  ),

  _btn_show_value(
    GfxUILayout(
      _btn_draw_curve.elementPosX(), (_ctrl_group.elementPosY() + (_ctrl_group.elementHeight() >> 1)),
      128, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Show value"
  ),


  _btn_draw_grid(
    GfxUILayout(
      (_btn_draw_curve.elementPosX()+_btn_draw_curve.elementWidth()+128), _ctrl_group.elementPosY(),
      128, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Draw grid"
  ),

  _btn_grid_lock_x(
    GfxUILayout(
      (_btn_draw_grid.elementPosX()+GFXUI_SF_CTRL_OFFSET_PX), (_ctrl_group.elementPosY() + (_ctrl_group.elementHeight() >> 1)),
      32, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "X"
  ),
  _btn_grid_lock_y(
    GfxUILayout(
      (_btn_grid_lock_x.elementPosX() + _btn_grid_lock_x.elementWidth()), _btn_grid_lock_x.elementPosY(),
      32, (_ctrl_group.elementHeight() >> 1),
      1, 1, 1, 1,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Y"
  ),

  _major_x_group(
    (_btn_grid_lock_y.elementPosX()+_btn_grid_lock_y.elementWidth()+48), _btn_grid_lock_y.elementPosY(),
    200, (_ctrl_group.elementHeight() >> 1)
  ),
  _major_y_group(
    _major_x_group.elementPosX(), (_ctrl_group.elementPosY() + (_ctrl_group.elementHeight() >> 1)),
    200, (_ctrl_group.elementHeight() >> 1)
  ),


  _slider_x_axis(
    GfxUILayout(
      internalPosX(), (_ctrl_group.elementPosY() - 20),  // Bottom-float pattern
      internalWidth(), 10,
      0, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty,
    (GFXUI_SLIDER_FLAG_MARK_ONLY)
  ),
  _graph(
    GfxUILayout(
      internalPosX(), internalPosY(),
      internalWidth(), (_slider_x_axis.elementPosY() - internalPosY()),
      0, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty,
    sf,
    (GFXUI_FLAG_TRACK_POINTER | GFXUI_FLAG_ALWAYS_REDRAW)
  ),
  _y_axis_min(T(0)), _y_axis_max(T(0))
{
  _ctrl_group.add_child(&_btn_autoscale_x);
  _ctrl_group.add_child(&_btn_autoscale_y);
  _ctrl_group.add_child(&_btn_draw_curve);
  _ctrl_group.add_child(&_btn_draw_grid);
  _ctrl_group.add_child(&_btn_show_value);
  _ctrl_group.add_child(&_btn_show_range_x);
  _ctrl_group.add_child(&_btn_show_range_y);
  _ctrl_group.add_child(&_btn_grid_lock_x);
  _ctrl_group.add_child(&_btn_grid_lock_y);
  _ctrl_group.add_child(&_major_x_group);
  _ctrl_group.add_child(&_major_y_group);
  _add_child(&_slider_x_axis);
  _add_child(&_ctrl_group);
  _add_child(&_graph);
  _graph.elementActive(true);
}


/* A high-cost pane for detailed examination and control over a TimeSeries. */
template <class T> class GfxUITimeSeriesDetail : public GfxUITabbedContentPane {
  public:
    GfxUITimeSeriesDetail(const GfxUILayout lay, const GfxUIStyle sty, TimeSeries<T>* sf, uint32_t f = 0) :
      GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
      _filter(sf),
      _filter_mirror(sf->windowSize()),
      _running_stdev(sf->windowSize()),
      _running_min(sf->windowSize()),
      _running_mean(sf->windowSize()),
      _running_max(sf->windowSize()),
      _skipped_samples(0),
      _pane_data(
        GfxUILayout(
          internalPosX(), (internalPosY() + _tab_bar.elementHeight()),
          internalWidth(), (internalHeight() - _tab_bar.elementHeight()),
          1, 0, 0, 0,   // Margins_px(t, b, l, r)
          0, 0, 0, 0               // Border_px(t, b, l, r)
        ),
        sty,
        sf,
        (GFXUI_SENFILT_FLAG_SHOW_RANGE | GFXUI_FLAG_TRACK_POINTER | GFXUI_SENFILT_FLAG_SHOW_VALUE | GFXUI_FLAG_ALWAYS_REDRAW)
      ),
      _pane_stats(
        GfxUILayout(
          internalPosX(), (internalPosY() + _tab_bar.elementHeight()),
          internalWidth(), (internalHeight() - _tab_bar.elementHeight()),
          1, 0, 0, 0,   // Margins_px(t, b, l, r)
          0, 0, 0, 0               // Border_px(t, b, l, r)
        ),
        sty,
        &_running_stdev,
        (GFXUI_SENFILT_FLAG_SHOW_RANGE | GFXUI_FLAG_TRACK_POINTER | GFXUI_SENFILT_FLAG_SHOW_VALUE | GFXUI_FLAG_ALWAYS_REDRAW)
      )
    {
      // Note our subordinate objects...
      //_pane_config.add_child(&_txt1);
      addTab("Data", &_pane_data, true);
      addTab("Stats", &_pane_stats);
    };

    ~GfxUITimeSeriesDetail() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    //virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    int fast_forward_data();


  protected:
    TimeSeries<T>* _filter;
    TimeSeries<T>  _filter_mirror;   // Decouples filter feed-rate from render-rate while keeping stats syncd.
    TimeSeries<float>  _running_stdev;
    TimeSeries<T>      _running_min;
    TimeSeries<float>  _running_mean;
    TimeSeries<T>      _running_max;
    uint32_t           _skipped_samples;
    GfxUITimeSeries<T> _pane_data;
    GfxUITimeSeries<float> _pane_stats;
    //GfxUIOptionsView     _pane_config;

    int8_t _filter_alignment_check();
};




/*
* All types used with this template have isomorphic notify behavior.
*/
template <class T> bool GfxUITimeSeries<T>::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      //change_log->insert(this, (int) GfxUIEvent::DRAG_START);
      ret = true;
      break;

    case GfxUIEvent::DRAG_START:
      //reposition(x, y);
      ret = true;
      break;

    case GfxUIEvent::MOVE_UP:
      _left_most_data_idx = strict_min(
        (uint32_t) (_left_most_data_idx + 20),
        (uint32_t) (_filter->windowSize() - internalWidth())
      );
      ret = true;
      break;

    case GfxUIEvent::MOVE_DOWN:
      _left_most_data_idx = (uint32_t) strict_max(
        ((int32_t) _left_most_data_idx - 20), (int32_t) 0
      );
      ret = true;
      break;

    case GfxUIEvent::RELEASE:
      //showValue(GfxUIEvent::TOUCH == GFX_EVNT);  // TODO: Implement with a button.
      //change_log->insert(this, (int) GfxUIEvent::DRAG_STOP);
      ret = true;
      break;
    default:
      break;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


/**
* Checks the derived filter sizes, types, and allocation status against the feed
*   filter, and makes any corrections required.
*
* @return -1 on failure, 0 on success.
*/
template <class T> int8_t GfxUITimeSeriesDetail<T>::_filter_alignment_check() {
  const uint32_t FEED_WIN_SZ = _filter->windowSize();
  int8_t ret = (_filter->initialized() && (0 < FEED_WIN_SZ)) ? 0 : -1;

  // Boundary checks...
  if ((0 == ret) && (FEED_WIN_SZ != _filter_mirror.windowSize())) {
    ret = _filter_mirror.windowSize(FEED_WIN_SZ);
  }
  if ((0 == ret) && (FEED_WIN_SZ != _running_stdev.windowSize())) {
    ret = _running_stdev.windowSize(FEED_WIN_SZ);
  }
  if ((0 == ret) && (FEED_WIN_SZ != _running_min.windowSize())) {
    ret = _running_min.windowSize(FEED_WIN_SZ);
  }
  if ((0 == ret) && (FEED_WIN_SZ != _running_mean.windowSize())) {
    ret = _running_mean.windowSize(FEED_WIN_SZ);
  }
  if ((0 == ret) && (FEED_WIN_SZ != _running_max.windowSize())) {
    ret = _running_max.windowSize(FEED_WIN_SZ);
  }

  // Allocation checks...
  if (0 == ret) {  ret = _filter_mirror.initialized() ? 0 : _filter_mirror.init();  }
  if (0 == ret) {  ret = _running_stdev.initialized() ? 0 : _running_stdev.init();  }
  if (0 == ret) {  ret = _running_min.initialized() ? 0 : _running_min.init();      }
  if (0 == ret) {  ret = _running_mean.initialized() ? 0 : _running_mean.init();    }
  if (0 == ret) {  ret = _running_max.initialized() ? 0 : _running_max.init();      }

  ret = (0 == ret) ? 0 : -1;
  return ret;
}


/**
* This should be called ahead of rendering (but in render's scope) to check how
*   much new data arrived in the filter since the last rendering. Then, the
*   mirrow filter will be updated and stats sequentially updated. This is very
*   expensive to do, but if it isn't done regularly, the stats filters will be
*   dropped, as they will not be reliable.
*
* @return Negative on failure, 0 on no update, or the number of new data-points.
*/
template <class T> int GfxUITimeSeriesDetail<T>::fast_forward_data() {
  int ret = -1;
  if (0 == _filter_alignment_check()) {
    const uint32_t SAMPLES_FEED   = _filter->totalSamples();
    const uint32_t SAMPLES_MIRROR = _filter_mirror.totalSamples() + _skipped_samples;
    ret--;
    if (SAMPLES_FEED > SAMPLES_MIRROR) {
      const uint32_t FEED_WIN_SZ = _filter->windowSize();
      const uint32_t FF_COUNT    = (SAMPLES_FEED - SAMPLES_MIRROR);
      ret--;
      if (FF_COUNT > _filter_mirror.windowSize()) {
        // Blow away the mirror, copy it fresh, and note how many samples we dropped.
        _skipped_samples = (_filter->totalSamples() - _filter_mirror.totalSamples());
        // The stats will be broken. Reset them so that it is obvious.
        _running_stdev.purge();
        _running_min.purge();
        _running_mean.purge();
        _running_max.purge();
      }
      // We have a current-enough copy of the data to ammend the mirror and
      //   keep our stats continuous. Update all stats filters sequentially.
      for (uint32_t i = 0; i < FF_COUNT; i++) {
        // Get the pointer to the next value in the feed filter...
        const T* FEED_MEM_PTR = _filter->memPtr() + (((FEED_WIN_SZ + _filter->lastIndex()) - (FF_COUNT - i)) % FEED_WIN_SZ);
        const T FEED_VALUE = *FEED_MEM_PTR;        // Get the next value from the filter...
        _filter_mirror.feedSeries(FEED_VALUE);     // ...insert it into the mirror...
        if (_filter_mirror.windowFull()) {         // ...and record the running stats if the mirror is full.
          _running_stdev.feedSeries(_filter_mirror.stdev());
          _running_min.feedSeries(_filter_mirror.minValue());
          _running_mean.feedSeries(_filter_mirror.mean());
          _running_max.feedSeries(_filter_mirror.maxValue());
        }
      }
      ret = FF_COUNT;
    }
    else if (SAMPLES_FEED == SAMPLES_MIRROR) {
      ret = 0;   // Mirror is current.
    }
    else {
      // The mirror contains data that was purged.
      _filter_mirror.purge();
      _running_stdev.purge();
      _running_min.purge();
      _running_mean.purge();
      _running_max.purge();
      _skipped_samples = (_filter->totalSamples() - _filter_mirror.totalSamples());
    }
  }
  return ret;
}


template <class T> int GfxUITimeSeriesDetail<T>::_render(UIGfxWrapper* ui_gfx) {
  fast_forward_data();
  return GfxUITabbedContentPane::_render(ui_gfx);
}


#endif  // __C3P_GFXUI_KIT_GRAPHING_H
