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


/*******************************************************************************
* Graphical tools for manipulating filters.
*******************************************************************************/

/* A basic pane that shows an annotated graph of a given SensorFilter. */
template <class T> class GfxUISensorFilter : public GfxUIElement {
  public:
    GfxUISensorFilter(const GfxUILayout lay, const GfxUIStyle sty, SensorFilter<T>* sf, uint32_t f = 0) : GfxUIElement(lay, sty, f | GFXUI_FLAG_ALWAYS_REDRAW), _filter(sf) {};
    ~GfxUISensorFilter() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    inline void showValue(bool x) {  _class_set_flag(GFXUI_SENFILT_FLAG_SHOW_VALUE, x); };
    inline bool showValue() {        return _class_flag(GFXUI_SENFILT_FLAG_SHOW_VALUE); };

    inline void showRange(bool x) {  _class_set_flag(GFXUI_SENFILT_FLAG_SHOW_RANGE, x); };
    inline bool showRange() {        return _class_flag(GFXUI_SENFILT_FLAG_SHOW_RANGE); };


  private:
    SensorFilter<T>* _filter;
    uint32_t _left_most_data_idx = 0;
};


/* A high-cost pane for detailed examination and control over a SensorFilter. */
template <class T> class GfxUITimeSeriesDetail : public GfxUITabbedContentPane {
  public:
    GfxUITimeSeriesDetail(const GfxUILayout lay, const GfxUIStyle sty, SensorFilter<T>* sf, uint32_t f = 0) :
      GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
      _filter(sf),
      _filter_mirror(sf->windowSize(), _filter->strategy()),
      _running_stdev(sf->windowSize(), FilteringStrategy::RAW),
      _running_min(sf->windowSize(), FilteringStrategy::RAW),
      _running_mean(sf->windowSize(), FilteringStrategy::RAW),
      _running_max(sf->windowSize(), FilteringStrategy::RAW),
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
      ),
      _pane_config(0, 0, 0, 0)
    {
      // Note our subordinate objects...
      //_pane_config.add_child(&_txt1);
      addTab("Data", &_pane_data, true);
      addTab("Stats", &_pane_stats);
      addTab("Config", &_pane_config);
    };

    ~GfxUITimeSeriesDetail() {};

    /* Implementation of GfxUIElement. */
    virtual int  _render(UIGfxWrapper* ui_gfx);
    //virtual bool _notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log);

    int fast_forward_data();


  protected:
    SensorFilter<T>* _filter;
    SensorFilter<T>  _filter_mirror;   // Decouples filter feed-rate from render-rate while keeping stats syncd.
    SensorFilter<float>  _running_stdev;
    SensorFilter<T>      _running_min;
    SensorFilter<float>  _running_mean;
    SensorFilter<T>      _running_max;
    uint32_t             _skipped_samples;
    GfxUISensorFilter<T> _pane_data;
    GfxUISensorFilter<float> _pane_stats;
    GfxUIGroup           _pane_config;
    //GfxUIOptionsView     _pane_config;

    int8_t _filter_alignment_check();
};




/*
* All types used with this template have isomorphic notify behavior.
*/
template <class T> bool GfxUISensorFilter<T>::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
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
        (uint32_t) (_left_most_data_idx + 10),
        (uint32_t) (_filter->windowSize() - internalWidth())
      );
      ret = true;
      break;

    case GfxUIEvent::MOVE_DOWN:
      _left_most_data_idx = (uint32_t) strict_max(
        (int32_t) (_left_most_data_idx - 10), (int32_t) 0
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
        _filter_mirror.feedFilter(FEED_VALUE);     // ...insert it into the mirror...
        if (_filter_mirror.windowFull()) {         // ...and record the running stats if the mirror is full.
          _running_stdev.feedFilter(_filter_mirror.stdev());
          _running_min.feedFilter(_filter_mirror.minValue());
          _running_mean.feedFilter(_filter_mirror.mean());
          _running_max.feedFilter(_filter_mirror.maxValue());
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
