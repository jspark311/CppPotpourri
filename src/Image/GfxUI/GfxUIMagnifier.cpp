/*
File:   GfxUIMagnifier.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIMagnifier
*******************************************************************************/

GfxUIMagnifier::GfxUIMagnifier(Image* src_img, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, (f | GFXUI_FLAG_ALWAYS_REDRAW)), _color(color), _src(src_img), _scale(2.0), _min_mag(1.0f), _max_mag(40.0f) {}


int GfxUIMagnifier::setBoands(float min_mag, float max_mag) {
  int ret = -1;
  if (max_mag > min_mag) {
    _min_mag = min_mag;
    _max_mag = max_mag;
    _scale = range_bind(_scale, _min_mag, _max_mag);
    ret = 0;
  }
  return ret;
}


int GfxUIMagnifier::_render(UIGfxWrapper* ui_gfx) {
  const int   INSET_FEED_SIZE_X = (_internal_Width()  / _scale);
  const int   INSET_FEED_SIZE_Y = (_internal_Height() / _scale);
  const int   INSET_FEED_OFFSET_X = (INSET_FEED_SIZE_X/2) + 1;
  const int   INSET_FEED_OFFSET_Y = (INSET_FEED_SIZE_Y/2) + 1;
  const uint  INSET_X_POS = (ui_gfx->img()->x() - elementWidth()) - 1;
  const uint  INSET_Y_POS = (ui_gfx->img()->y() - elementHeight()) - 1;
  const uint  INSET_FEED_X_POS  = (uint) range_bind((int) _pointer_x, INSET_FEED_OFFSET_X, (int) (ui_gfx->img()->x() - INSET_FEED_OFFSET_X)) - INSET_FEED_OFFSET_X;
  const uint  INSET_FEED_Y_POS  = (uint) range_bind((int) _pointer_y, INSET_FEED_OFFSET_Y, (int) (ui_gfx->img()->y() - INSET_FEED_OFFSET_Y)) - INSET_FEED_OFFSET_Y;
  reposition(INSET_X_POS, INSET_Y_POS);

  // Scale from the source image.
  ImageScaler scale_window(_src, ui_gfx->img(), _scale, INSET_FEED_X_POS, INSET_FEED_Y_POS, INSET_FEED_SIZE_X, INSET_FEED_SIZE_Y, INSET_X_POS, INSET_Y_POS);
  scale_window.apply();

  // Draw the frames and tracers.
  if (_class_flags() & GFXUI_MAGNIFIER_FLAG_SHOW_TRACERS) {
    ui_gfx->img()->drawLine(INSET_FEED_X_POS, (INSET_FEED_Y_POS + INSET_FEED_SIZE_Y), elementPosX(), (elementPosY() + elementHeight()), _color);
    ui_gfx->img()->drawLine((INSET_FEED_X_POS + INSET_FEED_SIZE_X), INSET_FEED_Y_POS, (elementPosX() + elementWidth()), elementPosY(), _color);
  }
  if (_class_flags() & GFXUI_MAGNIFIER_FLAG_SHOW_FEED_FRAME) {
    ui_gfx->img()->drawRect(INSET_FEED_X_POS, INSET_FEED_Y_POS, INSET_FEED_SIZE_X, INSET_FEED_SIZE_Y, _color);
  }
  return 1;
}


bool GfxUIMagnifier::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::MOVE_UP:
      if (_scale >= 1.0) {
        _scale += 1.0;
      }
      else {
        _scale = _scale * 2.0;
      }
      ret = true;
      break;

    case GfxUIEvent::MOVE_DOWN:
      if (_scale > 1.0) {
        _scale -= 1.0;
      }
      else {
        _scale = _scale * 0.5;
      }
      ret = true;
      break;

    default:
      break;
  }
  if (ret) {
    // Maximum magnification is 40x. Minimum magnification is 1x.
    _scale = range_bind(_scale, _min_mag, _max_mag);
    _need_redraw(true);
  }
  return ret;
}
