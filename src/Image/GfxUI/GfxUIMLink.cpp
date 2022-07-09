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
  _tab_bar(_internal_PosX(), _internal_PosY(), _internal_Width(), _internal_Height(), 0xCC99CC),
  _content_info(0, 0, 0, 0),
  _content_conf(0, 0, 0, 0),
  _content_msg(0, 0, 0, 0),
  _content_ses(0, 0, 0, 0),
  _btn_conf_syncast("Cast Sync",  0, 0, 60, 22, 0x9932CC, (_link->syncCast() ? GFXUI_BUTTON_FLAG_STATE : 0)),
  _btn_msg_send_sync("Send Sync", 0, 0, 60, 22, 0x9932CC, GFXUI_BUTTON_FLAG_MOMENTARY),
  _btn_ses_hangup("Hangup",       0, 0, 40, 22, 0x9932CC, GFXUI_BUTTON_FLAG_MOMENTARY),
  _txt(0, 0, _internal_Width(), (_internal_Height()-20), 0xCC99CC)
{
  // Note our subordinate objects...
  if (nullptr != _link->localIdentity()) {
    _content_conf.add_child(
      new GfxUIIdentity(
        _link->localIdentity(),
        _internal_Width()-150, 0,
        150, 80,
        0x20B2AA,
        (GFXUI_FLAG_DRAW_FRAME_L | GFXUI_FLAG_DRAW_FRAME_D)
      )
    );
  }
  _content_info.add_child(&_txt);
  _content_conf.add_child(&_btn_conf_syncast);
  _content_msg.add_child(&_btn_msg_send_sync);
  _content_ses.add_child(&_btn_ses_hangup);
  _tab_bar.addTab("Overview", &_content_info, true);
  _tab_bar.addTab("Conf", &_content_conf);
  _tab_bar.addTab("Messages", &_content_msg);
  _tab_bar.addTab("Counterparty", &_content_ses);
  _add_child(&_tab_bar);
}


int GfxUIMLink::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder _tmp_sbldr;
  switch (_tab_bar.activeTab()) {
    case 0:
      _txt.clear();
      _link->printDebug(&_tmp_sbldr);
      _txt.provideBuffer(&_tmp_sbldr);
      break;
    case 1:
      if (_btn_conf_syncast.pressed() ^ _link->syncCast()) {
        _link->syncCast(_btn_conf_syncast.pressed());
        //_btn_conf_syncast.pressed(_link->syncCast());
      }
      break;
    case 2:
      if (_btn_msg_send_sync.pressed()) {
      }
      break;
    case 3:
      if (_btn_ses_hangup.pressed()) {
        _link->hangup();
      }
      //_tmp_sbldr.concatf(ManuvrLink::sessionStateStr(_link->getState()));
      break;
    default:
      break;
  }
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
