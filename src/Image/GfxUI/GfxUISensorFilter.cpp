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
  const PixUInt I_W = internalWidth();
  PixUInt i_h = internalHeight();
  ui_gfx->img()->setTextSize(_style.text_size);
  //if ((_filter->dirty() | underPointer()) && _filter->windowFull()) {
  if (_filter->dirty() | underPointer()) {
    // The TimeSeries has a certain number of samples, and the rending is to be
    //   so wide. Take the smaller of the two, and that will be how many samples
    //   we want to render.
    const uint32_t  DATA_SIZE   = _filter->windowSize();
    const uint32_t  RENDER_SIZE = strict_min(DATA_SIZE, (uint32_t) I_W);
    // Determining the bounds of the sample window is subtle. First, lets take
    //   the frustum position from userspace, and anchor it to a location in the
    //   series. If autoscroll is enabled, things are much simpler because it is
    //   derived.
    if (_opt_autoscroll) {
      const uint32_t ABS_SAMPLE_IDX_LH = (_filter->totalSamples() - RENDER_SIZE);
      if (_filter->totalSamples() > RENDER_SIZE) {
        _left_most_data_idx = strict_min(
          (uint32_t) ((DATA_SIZE + _filter->lastIndex()) - RENDER_SIZE),
          (uint32_t) (_filter->windowSize() - RENDER_SIZE)
        );
      }

    }

    const uint32_t  LEFTMOST_SAMPLE_IDX  = (_filter->totalSamples() - (DATA_SIZE - _left_most_data_idx));
    trace_settings.offset_x = (_opt_x_labels_sample ? _filter->indexIsWhichSample(LEFTMOST_SAMPLE_IDX) : (LEFTMOST_SAMPLE_IDX % DATA_SIZE));

    //printf("%u  %u  %u \n", _left_most_data_idx, LEFTMOST_SAMPLE_IDX, _filter->indexIsWhichSample(LEFTMOST_SAMPLE_IDX));

    // Constrain the right-most sample index to not exceed the bounds of the
    //   TimeSeries. We are dealing with memory indicies here, So unless we want
    //   to risk wrapping the data within the render frustum, We want to limit
    //   how far to the right the frustum can move.
    // At the same time, we will need
    // With a constrained RH index, we can constrain the left-hand side, given the safe length.


    // With the correspondance between render and data established, we copy the
    //   data that will be used to draw the graph.
    uint32_t tmp_data[RENDER_SIZE];
    _filter->copyValueRange(tmp_data, RENDER_SIZE, LEFTMOST_SAMPLE_IDX, false);

    ImageGraph<uint32_t> graph(I_W, i_h);
    graph.fg_color          = 0xFFFFFFFF;
    trace_settings.color    = _style.color_active;
    trace_settings.dataset  = tmp_data;
    trace_settings.data_len = RENDER_SIZE;
    trace_settings.enabled  = true;
    // NOTE: offset_x only impacts render. Not reading of samples.

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
