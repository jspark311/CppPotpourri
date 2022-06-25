/*
File:   GfxUIIdentity.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIIdentity
*******************************************************************************/

GfxUIIdentity::GfxUIIdentity(Identity* id, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f)
  : GfxUIElement(x, y, w, h, f), _color(color), _ident(id),
  _tab_bar(_internal_PosX(), _internal_PosY(), _internal_Width(), 20, 0xCC99CC),
  _txt(_internal_PosX(), _internal_PosY() + _tab_bar.elementHeight(), _internal_Width(), h - _tab_bar.elementHeight(), 0xCC99CC)
{
  // Note our subordinate objects...
  _tab_bar.addTab("String", true);
  _tab_bar.addTab("Flags");
  _tab_bar.addTab("Conf");
  _add_child(&_tab_bar);
  _add_child(&_txt);
}

int GfxUIIdentity::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder _tmp_sbldr;
  _txt.clear();
  switch (_tab_bar.activeTab()) {
    case 0:
      _tmp_sbldr.concatf("%s\n", _ident->getHandle());
      _ident->toString(&_tmp_sbldr);
      break;
    case 1:
      _tmp_sbldr.concat("Flags\n");
      break;
    case 2:
      _tmp_sbldr.concat("Conf\n");
      break;
    default:
      break;
  }
  _txt.provideBuffer(&_tmp_sbldr);
  return 1;
}

bool GfxUIIdentity::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
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
