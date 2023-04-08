/*
File:   GfxUISensorFilter.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"

/*******************************************************************************
* GfxUISensorFilter
*******************************************************************************/

template <> int GfxUISensorFilter<uint32_t>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  uint32_t i_x = _internal_PosX();
  uint32_t i_y = _internal_PosY();
  uint16_t i_w = _internal_Width();
  uint16_t i_h = _internal_Height();
  ui_gfx->img()->setTextSize(_style.text_size);
  if ((_filter->dirty() | underPointer()) && _filter->windowFull()) {
    const uint32_t  DATA_SIZE = _filter->windowSize();
    const uint32_t  LAST_SIDX = _filter->lastIndex();
    const uint32_t  DATA_IDX  = (1 + LAST_SIDX + wrap_accounted_delta(DATA_SIZE, (uint32_t) i_w)) % DATA_SIZE;
    const uint32_t* F_MEM_PTR = _filter->memPtr();

    uint32_t tmp_data[DATA_SIZE];
    for (uint32_t i = 0; i < DATA_SIZE; i++) {
      tmp_data[i] = *(F_MEM_PTR + ((i + LAST_SIDX) % DATA_SIZE));
    }

    ImageGraph<uint32_t> graph(i_w, i_h);
    graph.fg_color            = 0xFFFFFFFF;
    graph.trace0.color        = _style.color_active;
    graph.trace0.show_y_range = showRange();
    graph.trace0.show_value   = underPointer();
    graph.trace0.dataset      = tmp_data;
    graph.trace0.data_len     = DATA_SIZE;
    graph.trace0.offset_x     = DATA_IDX;
    graph.trace0.enabled      = true;

    if (underPointer()) {
      graph.trace0.accented_idx = (_pointer_x - (i_x + 1));
    }

    graph.drawGraph(ui_gfx->img(), i_x, i_y);
    ret++;
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




template <> int GfxUISensorFilter<float>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  uint32_t i_x = _internal_PosX();
  uint32_t i_y = _internal_PosY();
  uint16_t i_w = _internal_Width();
  uint16_t i_h = _internal_Height();
  ui_gfx->img()->setTextSize(_style.text_size);
  if ((_filter->dirty() | underPointer()) && _filter->windowFull()) {
    const uint32_t  DATA_SIZE = _filter->windowSize();
    const uint32_t  LAST_SIDX = _filter->lastIndex();
    const uint32_t  DATA_IDX  = (1 + LAST_SIDX + wrap_accounted_delta(DATA_SIZE, (uint32_t) i_w)) % DATA_SIZE;
    const float*    F_MEM_PTR = _filter->memPtr();

    float tmp_data[DATA_SIZE];
    for (uint32_t i = 0; i < DATA_SIZE; i++) {
      tmp_data[i] = *(F_MEM_PTR + ((i + LAST_SIDX) % DATA_SIZE));
    }

    ImageGraph<float> graph(i_w, i_h);
    graph.fg_color            = 0xFFFFFFFF;
    graph.trace0.color        = _style.color_active;
    graph.trace0.show_y_range = showRange();
    graph.trace0.show_value   = underPointer();
    graph.trace0.dataset      = tmp_data;
    graph.trace0.data_len     = DATA_SIZE;
    graph.trace0.offset_x     = DATA_IDX;
    graph.trace0.enabled      = true;

    if (underPointer()) {
      graph.trace0.accented_idx = (_pointer_x - (i_x + 1));
    }

    graph.drawGraph(ui_gfx->img(), i_x, i_y);
    ret++;
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
