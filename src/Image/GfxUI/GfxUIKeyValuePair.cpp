/*
File:   GfxUIKeyValuePair.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIKeyValuePair
*******************************************************************************/
GfxUIKeyValuePair::GfxUIKeyValuePair(KeyValuePair* kvp, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _kvp(kvp)
{
  // TODO: Make a deep-copy?
};


int8_t GfxUIKeyValuePair::showKVP(KeyValuePair* new_kvp) {
  int8_t ret = -1;
  return ret;
}


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




/*******************************************************************************
* GfxUIKVPUtil
*******************************************************************************/

/**
* Constructor
*/
GfxUIKVPUtil::GfxUIKVPUtil(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _txt_serialized(
    GfxUILayout(
      internalPosX(), (internalPosY() + (internalHeight() >> 1)),
      internalWidth(), (internalHeight() >> 1),
      1, 1, 1, 1,
      0, 0, 0, 0               // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      _style.color_active,
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      _style.text_size
    ),
    (GFXUI_TXTAREA_FLAG_LINE_WRAP | GFXUI_TXTAREA_FLAG_WORD_WRAP | GFXUI_TXTAREA_FLAG_SCROLLABLE)
  ),
  _kvp_view(
    nullptr,
    GfxUILayout(
      internalPosX(), internalPosY(),
      internalWidth(), (internalHeight() >> 1),
      1, 1, 1, 1,
      0, 0, 0, 0               // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      _style.color_active,
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      _style.text_size
    ),
    (0)
  )
{
  _add_child(&_txt_serialized);
}


/**
* Destructor
*/
GfxUIKVPUtil::~GfxUIKVPUtil() {}



int GfxUIKVPUtil::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}


bool GfxUIKVPUtil::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
