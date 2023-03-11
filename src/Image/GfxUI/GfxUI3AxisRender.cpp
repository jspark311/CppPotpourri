/*
File:   GfxUI3AxisRender.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUI3AxisRender
*******************************************************************************/

int GfxUI3AxisRender::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}


bool GfxUI3AxisRender::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    default:
      return false;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}
