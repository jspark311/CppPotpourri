/*
File:   GfxUISensorFilter.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUITimeSeries
*******************************************************************************/

template <> int GfxUITimeSeries<uint32_t>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  PixUInt i_x = internalPosX();
  PixUInt i_y = internalPosY();
  PixUInt i_w = internalWidth();
  PixUInt i_h = internalHeight();
  ui_gfx->img()->setTextSize(_style.text_size);
  //if ((_filter->dirty() | underPointer()) && _filter->windowFull()) {
  if (_filter->dirty() | underPointer()) {
    const uint32_t  DATA_SIZE = _filter->windowSize();
    const uint32_t  RENDER_SIZE = strict_min((uint32_t) DATA_SIZE, (uint32_t) i_w);
    const uint32_t  LAST_SIDX = strict_min(DATA_SIZE, (_left_most_data_idx + RENDER_SIZE));
    // TODO: If the render wants strict window indicies for data, it should be
    //   handled by a modal boolean, or distinct functions for...
    //   a) copy from most-recent samples
    //   b) copy from first sample in RAM wrap will be plainly visible in the
    //      render, but some uses of TimeSeries are temporal snap-shots, and not
    //      bounded memories of an endless stream of samples.
    const uint32_t  DATA_IDX  = (LAST_SIDX - RENDER_SIZE);
    //const uint32_t  DATA_IDX  = (1 + LAST_SIDX + strict_abs_delta(DATA_SIZE, (uint32_t) RENDER_SIZE)) % DATA_SIZE;
    uint32_t tmp_data[RENDER_SIZE];

    // Option (a)
    const uint32_t* F_MEM_PTR = _filter->memPtr();
    for (uint32_t i = 0; i < RENDER_SIZE; i++) {
      tmp_data[i] = *(F_MEM_PTR + ((i + DATA_IDX) % DATA_SIZE));
    }

    // Option (b)
    //_filter->copyValues(tmp_data, RENDER_SIZE, false);

    ImageGraph<uint32_t> graph(i_w, i_h);
    graph.fg_color          = 0xFFFFFFFF;
    trace_settings.color    = _style.color_active;
    trace_settings.dataset  = tmp_data;
    trace_settings.data_len = RENDER_SIZE;
    trace_settings.enabled  = true;
    trace_settings.offset_x = DATA_IDX;  // This only impacts render. Not reading of samples.
    if (graph.trace0.copyFrom(&trace_settings)) {
      if (trackPointer() && underPointer()) {
        graph.trace0.accented_idx = (_pointer_x - (i_x + 1));
        ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(graph.trace0.color), ui_gfx->img()->convertColor(_style.color_bg));
      }

      graph.drawGraph(ui_gfx->img(), i_x, i_y);
      ret++;
    }
  }
  else if (_filter->initialized()) {
    if (!_filter->windowFull()) {
      StringBuilder temp_txt;
      ui_gfx->img()->setCursor(i_x + 1, i_y + 1);
      ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(_style.color_inactive), 0);
      temp_txt.concatf("%3u / %3u", _filter->lastIndex(), _filter->windowSize());
      ui_gfx->img()->writeString(&temp_txt);
      ret++;
    }
  }
  else {
    ui_gfx->img()->setCursor(i_x + 1, i_y + 1);
    ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(_style.color_active));
    ui_gfx->img()->writeString("Not init'd");
  }
  return ret;
}




template <> int GfxUITimeSeries<float>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  PixUInt i_x = internalPosX();
  PixUInt i_y = internalPosY();
  PixUInt i_w = internalWidth();
  PixUInt i_h = internalHeight();
  ui_gfx->img()->setTextSize(_style.text_size);
  if ((_filter->dirty() | underPointer()) && _filter->windowFull()) {
    const uint32_t  DATA_SIZE = _filter->windowSize();
    const uint32_t  LAST_SIDX = _filter->lastIndex();
    const uint32_t  DATA_IDX  = (1 + LAST_SIDX + strict_abs_delta(DATA_SIZE, (uint32_t) i_w)) % DATA_SIZE;
    const float*    F_MEM_PTR = _filter->memPtr();
    float tmp_data[DATA_SIZE];
    for (uint32_t i = 0; i < DATA_SIZE; i++) {
      tmp_data[i] = *(F_MEM_PTR + ((i + LAST_SIDX) % DATA_SIZE));
    }

    ImageGraph<float> graph(i_w, i_h);
    graph.fg_color            = 0xFFFFFFFF;
    trace_settings.dataset    = tmp_data;
    graph.trace0.data_len     = DATA_SIZE;
    graph.trace0.offset_x     = DATA_IDX;
    graph.trace0.enabled      = true;
    if (graph.trace0.copyFrom(&trace_settings)) {
      if (trackPointer() && underPointer()) {
        graph.trace0.accented_idx = (_pointer_x - (i_x + 1));
        ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(graph.trace0.color), ui_gfx->img()->convertColor(_style.color_bg));
      }
      graph.drawGraph(ui_gfx->img(), i_x, i_y);
      ret++;
    }
  }
  else if (_filter->initialized()) {
    if (!_filter->windowFull()) {
      StringBuilder temp_txt;
      ui_gfx->img()->setCursor(i_x + 1, i_y + 1);
      ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(_style.color_inactive), 0);
      temp_txt.concatf("%3u / %3u", _filter->lastIndex(), _filter->windowSize());
      ui_gfx->img()->writeString(&temp_txt);
      ret++;
    }
  }
  else {
    ui_gfx->img()->setCursor(i_x + 1, i_y + 1);
    ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(_style.color_active));
    ui_gfx->img()->writeString("Not init'd");
  }
  return ret;
}



/*******************************************************************************
* GfxUITimeSeriesDetail
*******************************************************************************/
