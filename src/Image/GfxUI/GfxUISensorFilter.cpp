/*
File:   GfxUISensorFilter.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "GfxUIKit.h"


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
  if (_filter->dirty()) {
    ui_gfx->drawGraph(
      i_x, i_y, strict_min((uint16_t) _filter->windowSize(), i_w), i_h,
      _style.color_active,
      true, showRange(), underPointer(),
      //true, showRange(), showValue(),
      _filter
    );
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


template <> bool GfxUISensorFilter<uint32_t>::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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


template <> int GfxUISensorFilter<float>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  uint32_t i_x = _internal_PosX();
  uint32_t i_y = _internal_PosY();
  uint16_t i_w = _internal_Width();
  uint16_t i_h = _internal_Height();
  ui_gfx->img()->setTextSize(_style.text_size);
  if (_filter->dirty()) {
    ui_gfx->drawGraph(
      i_x, i_y,
      strict_min((uint16_t) _filter->windowSize(), i_w), i_h,
      _style.color_active,
      true, showRange(), underPointer(),
      //true, showRange(), showValue(),
      _filter
    );
    ret++;
  }
  else if (_filter->initialized()) {
    if (!_filter->windowFull()) {
      StringBuilder temp_txt;
      ui_gfx->img()->setCursor(i_x + 1, i_y + 1);
      ui_gfx->img()->setTextSize(_style.text_size);
      ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(_style.color_inactive), 0);
      temp_txt.concatf("%3u / %3u", _filter->lastIndex(), _filter->windowSize());
      ui_gfx->img()->writeString(&temp_txt);
      ret++;
    }
  }
  else {
    ui_gfx->img()->setCursor(i_x + 1, i_y + 1);
    ui_gfx->img()->setTextSize(_style.text_size);
    ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(_style.color_active));
    ui_gfx->img()->writeString("Not init'd");
  }
  return ret;
}


template <> bool GfxUISensorFilter<float>::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
