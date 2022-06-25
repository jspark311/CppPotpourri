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
  if (_filter->dirty()) {
    ui_gfx->drawGraph(
      _x, _y, strict_min((uint16_t) _filter->windowSize(), _w), _h, _color,
      true, showRange(), showValue(),
      _filter
    );
    ret++;
  }
  else if (_filter->initialized()) {
    if (!_filter->windowFull()) {
      StringBuilder temp_txt;
      ui_gfx->img()->setCursor(_x + 1, _y + 1);
      ui_gfx->img()->setTextSize(0);
      ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x0000FFFF), 0);
      temp_txt.concatf("%3u / %3u", _filter->lastIndex(), _filter->windowSize());
      ui_gfx->img()->writeString(&temp_txt);
      ret++;
    }
  }
  else {
    ui_gfx->img()->setCursor(_x + 1, _y + 1);
    ui_gfx->img()->setTextSize(0);
    ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x000000FF));
    ui_gfx->img()->writeString("Not init'd");
  }
  return ret;
}


template <> bool GfxUISensorFilter<uint32_t>::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
    case GfxUIEvent::RELEASE:
      showValue(GfxUIEvent::TOUCH == GFX_EVNT);
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


template <> int GfxUISensorFilter<float>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (_filter->dirty()) {
    ui_gfx->drawGraph(
      _x, _y, strict_min((uint16_t) _filter->windowSize(), _w), _h, _color,
      true, showRange(), showValue(),
      _filter
    );
    ret++;
  }
  else if (_filter->initialized()) {
    if (!_filter->windowFull()) {
      StringBuilder temp_txt;
      ui_gfx->img()->setCursor(_x + 1, _y + 1);
      ui_gfx->img()->setTextSize(0);
      ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x0000FFFF), 0);
      temp_txt.concatf("%3u / %3u", _filter->lastIndex(), _filter->windowSize());
      ui_gfx->img()->writeString(&temp_txt);
      ret++;
    }
  }
  else {
    ui_gfx->img()->setCursor(_x + 1, _y + 1);
    ui_gfx->img()->setTextSize(0);
    ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x000000FF));
    ui_gfx->img()->writeString("Not init'd");
  }
  return ret;
}


template <> bool GfxUISensorFilter<float>::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
    case GfxUIEvent::RELEASE:
      showValue(GfxUIEvent::TOUCH == GFX_EVNT);
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
