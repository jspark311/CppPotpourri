/*
File:   GfxUIMLink.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIMLink
*******************************************************************************/
#if defined(CONFIG_MANUVR_M2M_SUPPORT)

GfxUIMLink::GfxUIMLink(ManuvrLink* l, uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f)
  : GfxUIElement(x, y, w, h, (f | GFXUI_FLAG_ALWAYS_REDRAW)), _link(l),
  _tab_bar(_internal_PosX(), _internal_PosY(), _internal_Width(), 20, 0xCC99CC),
  _txt(_internal_PosX(), _internal_PosY() + _tab_bar.elementHeight(), _internal_Width(), h - _tab_bar.elementHeight(), 0xCC99CC)
{
  // Note our subordinate objects...
  _tab_bar.addTab("Transport", true);
  _tab_bar.addTab("Session");
  _tab_bar.addTab("Messages");
  _tab_bar.addTab("Counterparty");
  _add_child(&_tab_bar);
  _add_child(&_txt);
}


int GfxUIMLink::_render(UIGfxWrapper* ui_gfx) {
  const uint32_t INTRNL_X = _internal_PosX();
  const uint32_t INTRNL_Y = _internal_PosY();
  const uint32_t INTRNL_W = _internal_Width();
  const uint32_t INTRNL_H = _internal_Height();
  StringBuilder _tmp_sbldr;
  _txt.clear();
  switch (_tab_bar.activeTab()) {
    case 0:
      _tmp_sbldr.concat("Transport\n");
      break;
    case 1:
      _tmp_sbldr.concat("Session\n");
      //_tmp_sbldr.concatf(ManuvrLink::sessionStateStr(_link->getState()));
      _link->printDebug(&_tmp_sbldr);
      break;
    case 2:
      _tmp_sbldr.concat("Messages\n");
      _link->printFSM(&_tmp_sbldr);
      break;
    case 3:
      _tmp_sbldr.concat("Counterparty\n");
      break;
    default:
      break;
  }
  _txt.provideBuffer(&_tmp_sbldr);
  return 1;
}

bool GfxUIMLink::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
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

#endif  // CONFIG_MANUVR_M2M_SUPPORT
