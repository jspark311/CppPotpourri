/*
File:   GfxUIStorage.cpp
Author: J. Ian Lindsay
Date:   2022.06.19

This is a view that should only be used to represent a Storage object, and the
  tool breakouts surrounding it. For Records, look elsewhere.
*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIStorage
*******************************************************************************/

GfxUIStorage::GfxUIStorage(Storage* storage, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f)
  : GfxUIElement(x, y, w, h, f), _storage(storage)
{}

int GfxUIStorage::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}

bool GfxUIStorage::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
