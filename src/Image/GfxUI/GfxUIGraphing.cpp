/*
File:   GfxUIGraphWithCtrl.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"

/*******************************************************************************
* GfxUIGraphWithCtrl
*******************************************************************************/

// TODO: For some reason I do not understand, calls to this function crash the
//   program when it is generalized into the template header, and the compiler
//   is allowed to autogenerate it.
template <> int GfxUIGraphWithCtrl<uint32_t>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 1;
  const PixUInt I_X = internalPosX();
  const PixUInt I_Y = internalPosY();
  const PixUInt I_W = internalWidth();
  const PixUInt I_H = internalHeight();

  _graph.showValue(_btn_show_value.pressed());
  _graph.drawCurve(_btn_draw_curve.pressed());
  _graph.showRangeX(_btn_show_range_x.pressed());
  _graph.showRangeY(_btn_show_range_y.pressed());
  _graph.graphAutoscaleX(_btn_autoscale_x.pressed());
  _graph.graphAutoscaleY(_btn_autoscale_y.pressed());
  _graph.drawGrid(_btn_draw_grid.pressed());
  _btn_grid_lock_x.elementActive(_btn_draw_grid.pressed());
  _btn_grid_lock_y.elementActive(_btn_draw_grid.pressed());
  _graph.lockGridX(_btn_grid_lock_x.pressed());
  _graph.lockGridY(_btn_grid_lock_y.pressed());
  _graph.autoscroll(_btn_autoscroll.pressed());
  _graph.xLabelsSample(_btn_x_labels_samples.pressed());

  const float M0 = ((float) _graph.firstIdxRendered() / (float) _graph.dataset()->windowSize());
  const float M1 = ((float) (_graph.firstIdxRendered() + _graph.trace_settings.data_len) / (float) _graph.dataset()->windowSize());
  _slider_x_axis.value(M0, M1);

  //ui_gfx->img()->setTextSize(_style.text_size);
  const PixUInt TXT_PIXEL_WIDTH  = ui_gfx->img()->getFontWidth();
  //const PixUInt TXT_PIXEL_HEIGHT = ui_gfx->img()->getFontHeight();
  //const PixUInt CTRL_BOX_X = _ctrl_group.elementPosX();
  //const PixUInt CTRL_BOX_Y = _ctrl_group.elementPosY();

  // TODO: Generallize into flow table.
  const PixUInt TXT_ROW_H   = (_ctrl_group.elementHeight() >> 1);
  const PixUInt TXT_ROW_0   = (_ctrl_group.elementPosY());
  const PixUInt TXT_ROW_1   = (TXT_ROW_0 + TXT_ROW_H);

  const PixUInt TXT_COL_W   = (11 * TXT_PIXEL_WIDTH);   // TODO: Derive
  const PixUInt TXT_COL_0   = (_btn_autoscale_x.elementPosX() - TXT_COL_W);
  const PixUInt TXT_COL_1   = (_btn_show_range_x.elementPosX() - TXT_COL_W);
  const PixUInt TXT_COL_2   = (_btn_grid_lock_x.elementPosX() - TXT_COL_W);
  const PixUInt TXT_COL_3   = (_major_x_group.elementPosX());

  const uint32_t CTRL_COLOR = (_ctrl_group.underPointer() ? 0xE0E0E0 : 0xa0a0a0);

  ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(CTRL_COLOR), ui_gfx->img()->convertColor(_style.color_bg));
  ui_gfx->img()->setCursor(TXT_COL_0, TXT_ROW_0);    ui_gfx->img()->writeString("Autoscale");
  ui_gfx->img()->setCursor(TXT_COL_1, TXT_ROW_1);    ui_gfx->img()->writeString("Show range");
  ui_gfx->img()->setCursor(TXT_COL_2, TXT_ROW_1);    ui_gfx->img()->writeString("Axis Lock");

  const uint32_t DIVS_COLOR_COLOR = (_graph.drawGrid() ? _style.color_active : _style.color_inactive);
  StringBuilder tmp_sb;
  tmp_sb.concatf("%5u", _graph.trace_settings.major_grid_x);
  ui_gfx->img()->setCursor(TXT_COL_3, TXT_ROW_0);
  ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(CTRL_COLOR), ui_gfx->img()->convertColor(_style.color_bg));
  ui_gfx->img()->writeString("Divs X: ");
  ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(DIVS_COLOR_COLOR), ui_gfx->img()->convertColor(_style.color_bg));
  ui_gfx->img()->writeString(&tmp_sb);
  tmp_sb.clear();

  tmp_sb.concatf("%5u", _graph.trace_settings.major_grid_y);
  ui_gfx->img()->setCursor(TXT_COL_3, TXT_ROW_1);
  ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(CTRL_COLOR), ui_gfx->img()->convertColor(_style.color_bg));
  ui_gfx->img()->writeString("Divs Y: ");
  ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(DIVS_COLOR_COLOR), ui_gfx->img()->convertColor(_style.color_bg));
  ui_gfx->img()->writeString(&tmp_sb);
  tmp_sb.clear();
  return ret;
}



template <> bool GfxUIGraphWithCtrl<uint32_t>::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  if (ret) {
    change_log->insert(this, (int) GFX_EVNT);
  }
    _need_redraw(true);
  return ret;
}
