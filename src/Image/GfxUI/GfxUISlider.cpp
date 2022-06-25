/*
File:   GfxUISlider.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUISlider
*******************************************************************************/

GfxUISlider::GfxUISlider(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, f), _color_marker(color) {}


int GfxUISlider::_render(UIGfxWrapper* ui_gfx) {
  if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
    ui_gfx->drawProgressBarV(
      _x, _y, _w, _h, _color_marker,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  else {
    ui_gfx->drawProgressBarH(
      _x, _y, _w, _h, _color_marker,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  return 1;
}


bool GfxUISlider::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
        const float PIX_POS_REL = y - _y;
        _percentage = 1.0f - strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float)_h)));
      }
      else {
        const float PIX_POS_REL = x - _x;
        _percentage = strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float)_w)));
      }
    case GfxUIEvent::RELEASE:
      return true;

    case GfxUIEvent::MOVE_UP:
      _percentage = strict_min(1.0, (_percentage + 0.01));
      return true;

    case GfxUIEvent::MOVE_DOWN:
      _percentage = strict_max(0.0, (_percentage - 0.01));
      return true;

    default:
      return false;
  }
}
