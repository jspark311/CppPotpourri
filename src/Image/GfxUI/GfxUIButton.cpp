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
  uint32_t current_color = elementActive() ? _color_active_on : 0x909090;
  ui_gfx->drawButton(
    _internal_PosX(), _internal_PosY(),
    _internal_Width(), _internal_Height(),
    current_color,
    pressed()
  );
  return 1;
}

bool GfxUIButton::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (!momentary()) {  _class_flip_flag(GFXUI_BUTTON_FLAG_STATE);  }
      else {               _class_set_flag(GFXUI_BUTTON_FLAG_STATE);   }
      ret = true;
      break;
    case GfxUIEvent::RELEASE:
      if (momentary()) {
        _class_clear_flag(GFXUI_BUTTON_FLAG_STATE);
        ret = true;
      }
      break;
    default:
      return false;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


/*******************************************************************************
* GfxUITextButton
*******************************************************************************/

int GfxUITextButton::_render(UIGfxWrapper* ui_gfx) {
  GfxUIButton::_render(ui_gfx);
  uint32_t current_color = elementActive() ? _color_active_on : 0x909090;
  ui_gfx->img()->setCursor(_internal_PosX()+3, _internal_PosY()+3);
  ui_gfx->img()->setTextColor(
    (pressed() ? 0 : current_color),
    (pressed() ? current_color : 0)
  );
  ui_gfx->img()->writeString(_TXT);
  return 1;
}
