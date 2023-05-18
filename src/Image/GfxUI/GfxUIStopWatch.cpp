/*
File:   GfxUIStopWatch.cpp
Author: J. Ian Lindsay
Date:   2023.05.18

*/

#include "../GfxUI.h"

/*******************************************************************************
* GfxUIStopWatch
*******************************************************************************/
/**
* Constructor
*/
GfxUIStopWatch::GfxUIStopWatch(const char* name, StopWatch* sw, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
  _name(name),
  _stopwatch(sw) {};


int GfxUIStopWatch::_render(UIGfxWrapper* ui_gfx) {
  uint32_t i_x = _internal_PosX();
  uint32_t i_y = _internal_PosY();
  uint16_t i_w = _internal_Width();
  uint16_t i_h = _internal_Height();

  ui_gfx->img()->fillRect(i_x, i_y, i_w, i_h, _style.color_bg);

  ui_gfx->img()->setTextSize(_style.text_size-1);
  const uint32_t SCALE_TXT_H_PIX = ui_gfx->img()->getFontHeight() + 2;
  ui_gfx->img()->setTextSize(_style.text_size);


  StringBuilder line;
  ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
  ui_gfx->img()->setCursor(i_x, i_y);
  line.concat(_name);
  ui_gfx->img()->writeString(&line);
  line.clear();

  const uint32_t NAME_OFFSET_PIX = 150;   // TODO: Arbitrary.

  uint32_t b_x = (i_x + NAME_OFFSET_PIX);
  uint16_t b_w = (i_w - NAME_OFFSET_PIX);
  uint16_t b_h = (i_h - SCALE_TXT_H_PIX);
  uint16_t s_y = (i_y + b_h);
  const uint32_t TIME_RANGE = (_stopwatch->worstTime() - _stopwatch->bestTime());
  const float PRCNT_MEAN = (float) (_stopwatch->meanTime() - _stopwatch->bestTime()) / (float) TIME_RANGE;
  const float PRCNT_LAST = (float) (_stopwatch->lastTime() - _stopwatch->bestTime()) / (float) TIME_RANGE;
  const uint32_t INFILL_WIDTH = (PRCNT_MEAN * (b_w - 2));

  ui_gfx->img()->fillRoundRect((b_x + 1), (i_y + 1), INFILL_WIDTH, (b_h - 2), 5, 0x606060);
  ui_gfx->img()->drawRoundRect(b_x, i_y, b_w, b_h, 5, 0xFFFFFF);

  ui_gfx->img()->setTextSize(_style.text_size-1);

  ui_gfx->img()->setCursor(b_x, s_y);
  ui_gfx->img()->setTextColor(0xFFFFFF);
  line.concatf("%u us", _stopwatch->bestTime());
  ui_gfx->img()->writeString(&line);
  line.clear();

  line.concatf("%u us", _stopwatch->worstTime());
  uint32_t str_bounds_x = 0;
  uint32_t str_bounds_y = 0;
  uint32_t str_bounds_w = 0;
  uint32_t str_bounds_h = 0;
  ui_gfx->img()->getTextBounds(&line, 0, 0, &str_bounds_x, &str_bounds_y, &str_bounds_w, &str_bounds_h);
  const uint32_t SCALE_TXT_WC_X_PIX = (b_x + (b_w - str_bounds_w));
  ui_gfx->img()->setCursor(SCALE_TXT_WC_X_PIX, s_y);
  ui_gfx->img()->writeString(&line);
  line.clear();

  ui_gfx->img()->setCursor((b_x + (PRCNT_MEAN * (b_w - 2))), s_y);
  ui_gfx->img()->setTextColor(0x606060);
  line.concatf("%u us", _stopwatch->meanTime());
  ui_gfx->img()->writeString(&line);
  line.clear();

  ui_gfx->img()->setCursor((b_x + (PRCNT_LAST * (b_w - 2))), s_y);
  ui_gfx->img()->setTextColor(0xF01010);
  line.concatf("%u us", _stopwatch->lastTime());
  ui_gfx->img()->writeString(&line);
  line.clear();

  return 1;
}

bool GfxUIStopWatch::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      _stopwatch->reset();
      ret = true;
      break;

    default:
      break;
  }
  if (ret) {
    change_log->insert(this, (int) GFX_EVNT);
    _need_redraw(true);
  }
  return ret;
}
