/*
File:   GfxUIOptionsView.cpp
Author: J. Ian Lindsay
Date:   2023.10.04

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIDataRecord
*******************************************************************************/
GfxUIOptionsView::GfxUIOptionsView(ConfRecord* conf_record, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)), _conf(conf_record)
{
};



int GfxUIOptionsView::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}


bool GfxUIOptionsView::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    default:
      break;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}