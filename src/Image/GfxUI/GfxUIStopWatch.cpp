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
  const PixUInt MIN_MAX_LR_MARGIN = 4;

  PixUInt i_x = internalPosX();
  PixUInt i_y = internalPosY();
  PixUInt i_w = internalWidth();
  PixUInt i_h = internalHeight();

  ui_gfx->img()->fillRect(i_x, i_y, i_w, i_h, _style.color_bg);

  ui_gfx->img()->setTextSize(_style.text_size-1);
  const PixUInt SCALE_TXT_H_PIX = ui_gfx->img()->getFontHeight() + 2;
  ui_gfx->img()->setTextSize(_style.text_size);
  const PixUInt NAME_TXT_H_PIX  = ui_gfx->img()->getFontHeight();

  StringBuilder line;
  ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
  ui_gfx->img()->setCursor(i_x, i_y);
  line.concat(_name);
  ui_gfx->img()->writeString(&line);
  line.clear();

  // Show the tag.
  ui_gfx->img()->setTextSize(_style.text_size-1);
  ui_gfx->img()->setCursor(i_x, (i_y + 1 + NAME_TXT_H_PIX));
  ui_gfx->img()->setTextColor(0xFFFFFF, _style.color_bg);
  line.concatf("Tag: 0x%08x", _stopwatch->tag());
  ui_gfx->img()->writeString(&line);
  line.clear();

  const PixUInt NAME_OFFSET_PIX = 150;   // TODO: Arbitrary.
  const PixUInt b_x = (i_x + NAME_OFFSET_PIX);
  const PixUInt b_w = (i_w - NAME_OFFSET_PIX);
  const PixUInt b_h = (i_h - SCALE_TXT_H_PIX);
  const PixUInt s_y = (i_y + b_h);

  if (0 < _stopwatch->executions()) {
    const uint32_t TIME_RANGE = (_stopwatch->worstTime() - _stopwatch->bestTime());
    const float PRCNT_MEAN = (float) (_stopwatch->meanTime() - _stopwatch->bestTime()) / (float) TIME_RANGE;
    const float PRCNT_LAST = (float) (_stopwatch->lastTime() - _stopwatch->bestTime()) / (float) TIME_RANGE;
    const PixUInt INFILL_WIDTH = (PRCNT_MEAN * (b_w - 2));

    ui_gfx->img()->fillRoundRect((b_x + 1), (i_y + 1), INFILL_WIDTH, (b_h - 2), 5, 0x505050);
    ui_gfx->img()->drawRoundRect(b_x, i_y, b_w, b_h, 5, 0xFFFFFF);

    ui_gfx->img()->setTextSize(_style.text_size-1);

    // Annotate the bar with the best and worst run times.
    const PixUInt MIN_MAX_Y_POS = i_y + (b_h >> 1) - (SCALE_TXT_H_PIX >> 1);
    const PixUInt TXT_PIXEL_WIDTH  = ui_gfx->img()->getFontWidth();
    line.concatf("%u us", _stopwatch->bestTime());

    ui_gfx->img()->setTextColor(0xFFFFFF, _style.color_bg);
    ui_gfx->img()->setCursor((b_x + MIN_MAX_LR_MARGIN), MIN_MAX_Y_POS);
    ui_gfx->img()->writeString(&line);
    line.clear();

    {
      line.concatf("%u us", _stopwatch->worstTime());
      const PixUInt LINE_WIDTH = ((TXT_PIXEL_WIDTH + 1) * line.length()) + MIN_MAX_LR_MARGIN;
      ui_gfx->img()->setCursor((b_x + (b_w - LINE_WIDTH)), MIN_MAX_Y_POS);
      ui_gfx->img()->writeString(&line);
      line.clear();
    }

    // Now, for the mean and previous run times...
    {
      line.concatf("%u us", _stopwatch->meanTime());
      ui_gfx->img()->setTextColor(0xa0a0a0, _style.color_bg);
      const PixUInt LINE_WIDTH = ((TXT_PIXEL_WIDTH + 1) * line.length()) + MIN_MAX_LR_MARGIN;
      const PixUInt SCALE_TXT_WC_X_PIX = (b_x + 1 + (b_w - LINE_WIDTH));
      const PixUInt CONSTRAINED_CURSOR_X = strict_min((PixUInt) (b_x + (PRCNT_MEAN * (b_w - 2))), SCALE_TXT_WC_X_PIX);
      ui_gfx->img()->setCursor(CONSTRAINED_CURSOR_X, s_y);
      ui_gfx->img()->writeString(&line);
      line.clear();
    }

    {
      line.concatf("%u us", _stopwatch->lastTime());
      ui_gfx->img()->setTextColor(0xF06060, _style.color_bg);
      const PixUInt LINE_WIDTH = ((TXT_PIXEL_WIDTH + 1) * line.length()) + MIN_MAX_LR_MARGIN;
      const PixUInt SCALE_TXT_WC_X_PIX = (b_x + (b_w - LINE_WIDTH));
      const PixUInt CONSTRAINED_CURSOR_X = strict_min((PixUInt) (b_x + (PRCNT_LAST * (b_w - 2))), SCALE_TXT_WC_X_PIX);
      ui_gfx->img()->setCursor(CONSTRAINED_CURSOR_X, s_y);
      ui_gfx->img()->writeString(&line);
      line.clear();
    }
  }
  else {
    ui_gfx->img()->setCursor(b_x, i_y);
    ui_gfx->img()->setTextColor(0xFFFFFF, _style.color_bg);
    line.concat("No data");
    ui_gfx->img()->writeString(&line);
    line.clear();
  }
  return 1;
}


bool GfxUIStopWatch::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
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
