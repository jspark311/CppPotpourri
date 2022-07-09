/*
File:   GfxUI.cpp
Author: J. Ian Lindsay
Date:   2022.05.29

This file contains the base implementation of GfxUIElement. All classes in the
  GfxUI feature block ultimately come down to this.
*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIElement Base Class
*******************************************************************************/

GfxUIElement::GfxUIElement(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f) :
  _x(x), _y(y), _w(w), _h(h),
  _mrgn_t(0), _mrgn_b(0), _mrgn_l(0), _mrgn_r(0),
  _flags(f | GFXUI_FLAG_NEED_RERENDER) {}



void GfxUIElement::muteRender(bool x) {
  _class_set_flag(GFXUI_FLAG_MUTE_RENDER, x);
  if (!x && (!_class_flag(GFXUI_FLAG_ALWAYS_REDRAW))) {
    // Un-muting the render will cause a redraw, if the class isn't already
    //   going to do it.
    _class_set_flag(GFXUI_FLAG_NEED_RERENDER);
  }
}



void GfxUIElement::reposition(uint32_t x, uint32_t y) {
  int32_t shift_x = x - _x;
  int32_t shift_y = y - _y;
  _x += shift_x;
  _y += shift_y;
  if (_children.hasNext()) {
    // There are child objects to relocate.
    const uint QUEUE_SIZE = _children.size();
    for (uint n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      ui_obj->reposition(ui_obj->elementPosX() + shift_x, ui_obj->elementPosY() + shift_y);
    }
  }
  _need_redraw(true);
}


int GfxUIElement::_add_child(GfxUIElement* chld) {
  return _children.insert(chld);
}


bool GfxUIElement::notify(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y) {
  bool ret = false;
  if (includesPoint(x, y) && elementActive()) {
    ret = _notify(GFX_EVNT, x, y);
    if (!ret) {
      ret = _notify_children(GFX_EVNT, x, y);
    }
  }
  return ret;
}


bool GfxUIElement::_notify_children(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y) {
  if (_children.hasNext()) {
    // There are child objects to notify.
    const uint QUEUE_SIZE = _children.size();
    for (uint n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      if (ui_obj->notify(GFX_EVNT, x, y)) {
        return true;
      }
    }
  }
  return false;
}


int GfxUIElement::render(UIGfxWrapper* ui_gfx, bool force) {
  int ret = 0;
  if (!muteRender()) {
    ret += _render_children(ui_gfx, force);
    if (_need_redraw() | force) {
      ret += _render(ui_gfx);
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_U) {
        ui_gfx->img()->drawFastHLine(_x, _y, _w, 0xFFFFFF);
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_D) {
        ui_gfx->img()->drawFastHLine(_x, (_y+(_h-1)), _w, 0xFFFFFF);
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_L) {
        ui_gfx->img()->drawFastVLine(_x, _y, _h, 0xFFFFFF);
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_R) {
        ui_gfx->img()->drawFastVLine((_x+(_w-1)), _y, _h, 0xFFFFFF);
      }
      _need_redraw(false);
    }
  }
  return ret;
};



int GfxUIElement::_render_children(UIGfxWrapper* ui_gfx, bool force) {
  int ret = 0;
  if (_children.hasNext()) {
    // There are child objects to render.
    const uint QUEUE_SIZE = _children.size();
    for (uint n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      ret += ui_obj->render(ui_gfx, force);
    }
  }
  return ret;
}
