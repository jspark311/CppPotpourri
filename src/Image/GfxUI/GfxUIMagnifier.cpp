/*
File:   GfxUIMagnifier.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIMagnifier
*******************************************************************************/

GfxUIMagnifier::GfxUIMagnifier(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, f), _color(color)
{
  _scale = 0.5;
}


int GfxUIMagnifier::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}

bool GfxUIMagnifier::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::MOVE_UP:
      _scale = strict_min(1.0, (_scale + 0.01));
      return true;

    case GfxUIEvent::MOVE_DOWN:
      _scale = strict_max(0.01, (_scale - 0.01)); // TODO: Need a more appropriate bounding.
      return true;

    default:
      break;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}
