/*
File:   GfxUISlider.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUISlider
*******************************************************************************/

int GfxUISlider::_render(UIGfxWrapper* ui_gfx) {
  uint32_t i_x = internalPosX();
  uint32_t i_y = internalPosY();
  uint16_t i_w = internalWidth();
  uint16_t i_h = internalHeight();
  if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
    ui_gfx->drawProgressBarV(
      i_x, i_y, i_w, i_h, _style.color_active,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  else {
    ui_gfx->drawProgressBarH(
      i_x, i_y, i_w, i_h, _style.color_active,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  return 1;
}


bool GfxUISlider::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  float tmp_percentage = PI;  // Something clearly too big to be valid.

  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      change_log->insert(this, (int) GfxUIEvent::DRAG_START);
      // NOTE: No break;
    case GfxUIEvent::DRAG_START:
      if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
        const float PIX_POS_REL = y - internalPosY();
        tmp_percentage = 1.0f - strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float) internalHeight())));
      }
      else {
        const float PIX_POS_REL = x - internalPosX();
        tmp_percentage = strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float) internalWidth())));
      }
      ret = true;
      break;

    case GfxUIEvent::RELEASE:
      change_log->insert(this, (int) GfxUIEvent::DRAG_STOP);
      ret = true;
      break;

    case GfxUIEvent::MOVE_UP:
      tmp_percentage = strict_min(1.0, (_percentage + 0.01));
      change_log->insert(this, (int) GFX_EVNT);
      ret = true;
      break;

    case GfxUIEvent::MOVE_DOWN:
      tmp_percentage = strict_max(0.0, (_percentage - 0.01));
      change_log->insert(this, (int) GFX_EVNT);
      ret = true;
      break;

    default:
      return false;
  }

  if ((1.0 >= tmp_percentage) && (_percentage != tmp_percentage)) {
    _percentage = tmp_percentage;
    change_log->insert(this, (int) GfxUIEvent::VALUE_CHANGE);
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}
