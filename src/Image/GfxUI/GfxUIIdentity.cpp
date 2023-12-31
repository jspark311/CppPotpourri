/*
File:   GfxUIIdentity.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIIdentity
*******************************************************************************/

GfxUIIdentity::GfxUIIdentity(const GfxUILayout lay, const GfxUIStyle sty, Identity* id, uint32_t f) :
  GfxUIElement(lay, sty, f), _ident(id),
  _txt_handle(
    GfxUILayout(
      internalPosX(), internalPosY(),
      internalWidth(), (sty.text_size * 8),  // TODO: Better, but still arbitrary.
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
    )
  ),
  _txt_format(
    GfxUILayout(
      internalPosX(), (_txt_handle.elementPosY() + _txt_handle.elementHeight()),
      internalWidth(), (sty.text_size * 8),  // TODO: Better, but still arbitrary.
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
    )
  ),
  _txt_meta(
    GfxUILayout(
      _txt_format.elementPosX(), (_txt_format.elementPosY() + _txt_format.elementHeight()),
      internalWidth(), (sty.text_size * 8),  // TODO: Better, but still arbitrary.
      //(_txt_format.elementPosX() + _txt_format.elementWidth()), (_txt_format.elementPosY() + _txt_format.elementHeight()),
      //(internalWidth()-30), (sty.text_size * 8),  // TODO: Better, but still arbitrary.
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
    )
  ),
  _flag_render(0, 0, 0, 0)
{
  // Note our subordinate objects...
  _add_child(&_txt_handle);
  _add_child(&_txt_format);
  _add_child(&_txt_meta);
  _add_child(&_flag_render);
}


int GfxUIIdentity::_render(UIGfxWrapper* ui_gfx) {
  _txt_handle.clear();
  _txt_format.clear();
  _txt_meta.clear();
  if (nullptr != _ident) {
    StringBuilder _tmp_sbldr;
    _tmp_sbldr.concatf("%s\n", _ident->getHandle());
    _txt_handle.pushBuffer(&_tmp_sbldr);

    _tmp_sbldr.concatf("%s\n", Identity::identityTypeString(_ident->identityType()));
    _txt_format.pushBuffer(&_tmp_sbldr);

    _ident->toString(&_tmp_sbldr);
    _txt_meta.pushBuffer(&_tmp_sbldr);
  }
  return 1;
}

bool GfxUIIdentity::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
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
