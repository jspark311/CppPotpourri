/*
File:   GfxUIUART.cpp
Author: J. Ian Lindsay
Date:   2024.07.24

*/

#include "../GfxUI.h"
#include "../../BusQueue/UARTAdapter.h"


/**
* Constructor
*/
GfxUIUART::GfxUIUART(UARTAdapter* u, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
  _uart(u) {};



int GfxUIUART::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  const PixUInt I_X = internalPosX();
  const PixUInt I_Y = internalPosY();
  const PixUInt I_W = internalWidth();
  const PixUInt I_H = internalHeight();
  ui_gfx->img()->fillRect(I_X, I_Y, I_W, I_H, _style.color_bg);
  ui_gfx->img()->setCursor(I_X, I_Y);
  ui_gfx->img()->setTextSize(_style.text_size);
    const PixUInt TXT_PIXEL_WIDTH  = ui_gfx->img()->getFontWidth();
    const PixUInt TXT_PIXEL_HEIGHT = ui_gfx->img()->getFontHeight();
    const PixUInt LINE_H_DELTA = (TXT_PIXEL_HEIGHT + _style.text_size);
    //const PixUInt LABEL_COL_WIDTH = ((max_label_len + 1) * TXT_PIXEL_WIDTH);
    const PixUInt LABEL_COL_WIDTH = (40 * TXT_PIXEL_WIDTH);

  if (_uart) {
    StringBuilder output_lines;
    ui_gfx->img()->setTextColor((_uart->initialized() ? _style.color_active : _style.color_inactive), _style.color_bg);
    //ui_gfx->img()->writeString(_uart->path());
    if (_uart->initialized()) {
      UARTOpts* opts = _uart->uartOpts();
      output_lines.concatf("%ubps %u-%u-%u-%u", opts->bitrate, opts->start_bits, opts->bit_per_word, opts->parity, opts->stop_bits);
      output_lines.concatf("  Flow CTRL:    %u", UARTAdapter::flowCtrlStr(opts->flow_control));
      output_lines.concatf("  Xon/Xoff:     %u", opts->xon_char, opts->xoff_char);
      output_lines.concatf("  RX pending:   %u", _uart->pendingRxBytes());
      output_lines.concatf("  lastRXTime:   %u ms ago", delta_assume_wrap(millis(), (long unsigned) _uart->lastRXTime()));
      output_lines.concatf("  TX pending:   %u", _uart->pendingTxBytes());
      output_lines.concatf("  TX available: %u", _uart->bufferAvailable());
    }
    uint32_t i = 0;
    while (output_lines.count() > 0) {
      const PixUInt THIS_LINE_Y = (I_Y + (i++ * LINE_H_DELTA));
      ui_gfx->img()->setCursor(I_X, THIS_LINE_Y);
      ui_gfx->img()->writeString(output_lines.position(0));
      output_lines.drop_position(0);
    }
    ret = 1;
  }
  else {
    ui_gfx->img()->writeString("No UART");
  }
  return ret;
}



bool GfxUIUART::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
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
