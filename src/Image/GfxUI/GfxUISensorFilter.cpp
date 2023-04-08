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
    graph.trace0.enabled      = true;
    graph.drawGraph(ui_gfx->img(), i_x, i_y);


    if (underPointer()) {
      float v_scale = graph.trace0.v_scale;
      const uint32_t PIXEL_IDX = (_pointer_x - (i_x + 1));
      const uint32_t DATA_IDX  = (PIXEL_IDX + 1 + LAST_SIDX + wrap_accounted_delta(DATA_SIZE, (uint32_t) i_w)) % DATA_SIZE;
      if ((PIXEL_IDX < DATA_SIZE) & (0.0f < v_scale)) {
        const uint32_t DATA_VALUE = *(F_MEM_PTR + DATA_IDX);

        uint32_t tmp_y = i_y + (i_h - strict_min((uint32_t) (DATA_VALUE / v_scale), (uint32_t) i_h));
        ui_gfx->img()->drawFastVLine(_pointer_x, i_y, i_h, _style.color_active);

        const uint32_t POINT_SIZE = 3;
        uint32_t point_x = (_pointer_x - POINT_SIZE);
        uint32_t point_y = (tmp_y - POINT_SIZE);
        uint32_t point_h = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.
        uint32_t point_w = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.
        if (point_x < (i_x + 1)) {   // TODO: +1 because of the rendering of the vertical axis.
          // Point overflowing the left-hand element boundary?
          point_w = point_w - ((i_x + 1) - point_x);
          point_x = (i_x + 1);
        }
        else if ((point_x + point_w) > (i_x + i_w)) {
          // Point overflowing the right-hand element boundary?
          point_w = (i_x + i_w) - point_x;
        }
        if (point_y < i_y) {
          // Point overflowing the upper element boundary?
          point_h = point_h - (i_y - point_y);
          point_y = i_y;
        }
        else if ((point_y + point_h) > (i_y + i_h)) {
          // Point overflowing the lower element boundary?
          point_h = point_h - ((point_y + point_h) - (i_y + i_h));
        }
        ui_gfx->img()->fillRect(point_x, point_y, point_w, point_h, _style.color_active);

        const uint16_t TXT_PIXEL_HEIGHT = ui_gfx->img()->getFontHeight();
        StringBuilder temp_txt;
        uint32_t txt_x = (_pointer_x + 1);
        uint32_t txt_y = ((_pointer_y - i_y) > TXT_PIXEL_HEIGHT) ? (_pointer_y - TXT_PIXEL_HEIGHT) : i_y;
        uint32_t txt_w = 0;
        uint32_t txt_h = 0;
        temp_txt.concatf("%u: %u", DATA_IDX, DATA_VALUE);
        ui_gfx->img()->getTextBounds(&temp_txt, txt_x, txt_y, &txt_x, &txt_y, &txt_w, &txt_h);
        if ((txt_w + txt_x) > (i_x + i_w)) {
          // If the string would overflow the element's right-hand boundary,
          //   render it on the left-hand side of the indicator line.
          txt_x = _pointer_x - txt_w;
          if (txt_x < i_x) {
            // If we adjusted the text offset, and it now lies outside the
            //   left-hand boundary of the element, set the text to be at the
            //   boundary, and what happens happens.
            txt_x = i_x;
          }
        }

        if (txt_y < i_y) {
          // If the string would overflow the element's upper boundary,
          //   render it on the element's ceiling.
          txt_y = i_y;
        }
        else if ((txt_h + txt_y) > (i_y + i_h)) {
          // If the string would overflow the element's lower boundary,
          //   render it on the element's floor.
          txt_y = ((i_y + i_h) - TXT_PIXEL_HEIGHT);
        }

        ui_gfx->img()->setCursor(txt_x, txt_y);
        ui_gfx->img()->writeString(&temp_txt);
      }
    }
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
    ui_gfx->drawGraph(
      i_x, i_y, strict_min((uint16_t) _filter->windowSize(), i_w), i_h,
      _style.color_active,
      true, showRange(), underPointer(),
      //true, showRange(), showValue(),
      _filter
    );
    if (underPointer()) {
      const uint32_t DATA_SIZE = _filter->windowSize();
      const uint32_t LAST_SIDX = _filter->lastIndex();
      const float*   F_MEM_PTR = _filter->memPtr();

      float v_max = 0.0;
      float v_min = 0.0;
      for (uint32_t i = 0; i < LAST_SIDX; i++) {
        float tmp = *(F_MEM_PTR + i);
        v_max = strict_max(v_max, tmp);
        v_min = strict_min(v_min, tmp);
      }
      float v_scale = (v_max - v_min) / (float) i_h;
      const uint32_t PIXEL_IDX = (_pointer_x - (i_x + 1));
      const uint32_t DATA_IDX  = (PIXEL_IDX + 1 + LAST_SIDX + wrap_accounted_delta(DATA_SIZE, (uint32_t) i_w)) % DATA_SIZE;
      if ((PIXEL_IDX < DATA_SIZE) & (0.0f < v_scale)) {
        const float DATA_VALUE = *(F_MEM_PTR + DATA_IDX);

        uint32_t tmp_y = i_y + (i_h - strict_min((uint32_t) (DATA_VALUE / v_scale), (uint32_t) i_h));
        ui_gfx->img()->drawFastVLine(_pointer_x, i_y, i_h, _style.color_active);

        const uint32_t POINT_SIZE = 3;
        uint32_t point_x = (_pointer_x - POINT_SIZE);
        uint32_t point_y = (tmp_y - POINT_SIZE);
        uint32_t point_h = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.
        uint32_t point_w = ((POINT_SIZE << 1) + 1);  // Ensure an odd number.
        if (point_x < (i_x + 1)) {   // TODO: +1 because of the rendering of the vertical axis.
          // Point overflowing the left-hand element boundary?
          point_w = point_w - ((i_x + 1) - point_x);
          point_x = (i_x + 1);
        }
        else if ((point_x + point_w) > (i_x + i_w)) {
          // Point overflowing the right-hand element boundary?
          point_w = (i_x + i_w) - point_x;
        }
        if (point_y < i_y) {
          // Point overflowing the upper element boundary?
          point_h = point_h - (i_y - point_y);
          point_y = i_y;
        }
        else if ((point_y + point_h) > (i_y + i_h)) {
          // Point overflowing the lower element boundary?
          point_h = point_h - ((point_y + point_h) - (i_y + i_h));
        }
        ui_gfx->img()->fillRect(point_x, point_y, point_w, point_h, _style.color_active);

        const uint16_t TXT_PIXEL_HEIGHT = ui_gfx->img()->getFontHeight();
        StringBuilder temp_txt;
        uint32_t txt_x = (_pointer_x + 1);
        uint32_t txt_y = ((_pointer_y - i_y) > TXT_PIXEL_HEIGHT) ? (_pointer_y - TXT_PIXEL_HEIGHT) : i_y;
        uint32_t txt_w = 0;
        uint32_t txt_h = 0;
        temp_txt.concatf("%u: %.3f", DATA_IDX, DATA_VALUE);
        ui_gfx->img()->getTextBounds(&temp_txt, txt_x, txt_y, &txt_x, &txt_y, &txt_w, &txt_h);
        if ((txt_w + txt_x) > (i_x + i_w)) {
          // If the string would overflow the element's right-hand boundary,
          //   render it on the left-hand side of the indicator line.
          txt_x = _pointer_x - txt_w;
          if (txt_x < i_x) {
            // If we adjusted the text offset, and it now lies outside the
            //   left-hand boundary of the element, set the text to be at the
            //   boundary, and what happens happens.
            txt_x = i_x;
          }
        }

        if (txt_y < i_y) {
          // If the string would overflow the element's upper boundary,
          //   render it on the element's ceiling.
          txt_y = i_y;
        }
        else if ((txt_h + txt_y) > (i_y + i_h)) {
          // If the string would overflow the element's lower boundary,
          //   render it on the element's floor.
          txt_y = ((i_y + i_h) - TXT_PIXEL_HEIGHT);
        }

        ui_gfx->img()->setCursor(txt_x, txt_y);
        ui_gfx->img()->writeString(&temp_txt);
      }
    }
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
