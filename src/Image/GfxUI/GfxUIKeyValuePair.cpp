/*
File:   GfxUIKeyValuePair.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIKeyValuePair
*******************************************************************************/
GfxUIKeyValuePair::GfxUIKeyValuePair(const GfxUILayout lay, const GfxUIStyle sty, GfxUIKeyValuePair* kvp, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _kvp(kvp)
{
  // TODO: Make a deep-copy?
};


int GfxUIKeyValuePair::_render(UIGfxWrapper* ui_gfx) {

  return 1;
}


bool GfxUIKeyValuePair::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
