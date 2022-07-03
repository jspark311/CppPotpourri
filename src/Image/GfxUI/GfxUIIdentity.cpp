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
  _tab_bar(_internal_PosX(), _internal_PosY(), _internal_Width(), _internal_Height(), color),
  _txt0(0, 0, 0, 0, color),
  _txt1(0, 0, 0, 0, color),
  _txt2(0, 0, 0, 0, color)
{
  // Note our subordinate objects...
  _tab_bar.addTab("String", &_txt0, true);
  _tab_bar.addTab("Flags", &_txt1);
  _tab_bar.addTab("Conf", &_txt2);
  _add_child(&_tab_bar);
}


int GfxUIIdentity::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder _tmp_sbldr;
  switch (_tab_bar.activeTab()) {
    case 0:
      _txt0.clear();
      _tmp_sbldr.concatf("%s\n", _ident->getHandle());
      _ident->toString(&_tmp_sbldr);
      _txt0.provideBuffer(&_tmp_sbldr);
      break;
    case 1:
      _txt1.clear();
      _tmp_sbldr.concat("Flags\n");
      _txt1.provideBuffer(&_tmp_sbldr);
      break;
    case 2:
      _txt2.clear();
      _tmp_sbldr.concat("Conf\n");
      _txt2.provideBuffer(&_tmp_sbldr);
      break;
    default:
      break;
  }
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
