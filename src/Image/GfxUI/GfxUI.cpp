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

/* Simple constructor with discrete paramters. */
GfxUIElement::GfxUIElement(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f) :
  GfxUILayout(
    x, y, w, h,
    0, 0, 0, 0,
    0, 0, 0, 0
  ),
  _style(),
  _flags(f | GFXUI_FLAG_NEED_RERENDER)
{
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_U)) _bordr_t = 1;   // TODO: Temporary hack until styles are in-use.
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_D)) _bordr_b = 1;   // TODO: Temporary hack until styles are in-use.
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_L)) _bordr_l = 1;   // TODO: Temporary hack until styles are in-use.
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_R)) _bordr_r = 1;   // TODO: Temporary hack until styles are in-use.
}


/* Constructor with fully-specified paramters. */
GfxUIElement::GfxUIElement(const GfxUILayout layout, const GfxUIStyle style, uint32_t f) :
  GfxUILayout(layout),
  _style(style),
  _flags(f | GFXUI_FLAG_NEED_RERENDER) {}

/* Constructor with fully-specified paramters. */
GfxUIElement::GfxUIElement(GfxUILayout* layout, GfxUIStyle* style, uint32_t f) :
  GfxUILayout(layout),
  _style(style),
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
    const uint32_t QUEUE_SIZE = _children.size();
    for (uint32_t n = 0; n < QUEUE_SIZE; n++) {
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
    const uint32_t QUEUE_SIZE = _children.size();
    for (uint32_t n = 0; n < QUEUE_SIZE; n++) {
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
      const uint32_t COLOR = _style.color_border;
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_U) {
        for (uint8_t i = 0; i < _bordr_t; i++) {
          ui_gfx->img()->drawFastHLine(_x, (_y+i), _w, COLOR);
        }
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_D) {
        for (uint8_t i = 0; i < _bordr_b; i++) {
          ui_gfx->img()->drawFastHLine(_x, (_y+(_h-(1+i))), _w, COLOR);
        }
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_L) {
        for (uint8_t i = 0; i < _bordr_l; i++) {
          ui_gfx->img()->drawFastVLine((_x+i), _y, _h, COLOR);
        }
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_R) {
        for (uint8_t i = 0; i < _bordr_r; i++) {
          ui_gfx->img()->drawFastVLine((_x+(_w-(1+i))), _y, _h, COLOR);
        }
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
    const uint32_t QUEUE_SIZE = _children.size();
    for (uint32_t n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      ret += ui_obj->render(ui_gfx, force);
    }
  }
  return ret;
}
