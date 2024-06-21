/*
File:   GfxUIButton.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIButton
*******************************************************************************/

int GfxUIButton::_render(UIGfxWrapper* ui_gfx) {
  uint32_t current_color = elementActive() ? _style.color_active : _style.color_inactive;
  uint32_t current_pressed = pressed() ? current_color : _style.color_bg;
  uint32_t i_x = internalPosX();
  uint32_t i_y = internalPosY();
  uint16_t i_w = internalWidth();
  uint16_t i_h = internalHeight();
  Image* img = ui_gfx->img();
  const uint32_t ELEMENT_RADIUS = 4;
  img->fillRoundRect(i_x, i_y, i_w, i_h, ELEMENT_RADIUS, current_pressed);
  img->drawRoundRect(i_x, i_y, i_w, i_h, ELEMENT_RADIUS, _style.color_border);

  return 1;
}


bool GfxUIButton::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (momentary()) {
        pressed(true);
        change_log->insert(this, (int) GfxUIEvent::VALUE_CHANGE);
      }
      ret = true;
      break;

    case GfxUIEvent::RELEASE:
      if (momentary()) {
        pressed(false);
        change_log->insert(this, (int) GfxUIEvent::VALUE_CHANGE);
      }
      else {
        pressed(!pressed());
        change_log->insert(this, (int) GfxUIEvent::VALUE_CHANGE);
      }
      ret = true;
      break;

    default:
      return false;
  }
  if (ret) {
    change_log->insert(this, (int) GFX_EVNT);
    _need_redraw(true);
  }
  return ret;
}



/*******************************************************************************
* GfxUITextButton
*******************************************************************************/

int GfxUITextButton::_render(UIGfxWrapper* ui_gfx) {
  GfxUIButton::_render(ui_gfx);
  uint32_t current_color = elementActive() ? _style.color_active : _style.color_inactive;
  ui_gfx->img()->setTextSize(_style.text_size);
  ui_gfx->img()->setCursor(internalPosX()+3, internalPosY()+3);   // TODO: Implement better text justification.
  ui_gfx->img()->setTextColor(
    (pressed() ? _style.color_bg : current_color),
    (pressed() ? current_color : _style.color_bg)
  );
  ui_gfx->img()->writeString(_TXT);
  return 1;
}
