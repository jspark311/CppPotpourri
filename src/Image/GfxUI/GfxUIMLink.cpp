/*
File:   GfxUIMLink.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIMLink
*******************************************************************************/
#if defined(CONFIG_C3P_M2M_SUPPORT)

GfxUIMLink::GfxUIMLink(const GfxUILayout lay, const GfxUIStyle sty, M2MLink* l, uint32_t f) :
  GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
  _link(l),
  _content_info(0, 0, 0, 0),
  _content_conf(0, 0, 0, 0),
  _content_msg(0, 0, 0, 0),
  _content_ses(0, 0, 0, 0),
  _btn_conf_syncast(
    GfxUILayout(
      internalPosX(), (internalPosY() + 30),
      130, ((sty.text_size * 8) + 12),  // TODO: Better, but still arbitrary.
      2, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      0x20B2AA,   // active
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      2           // t_size
    ),
    "Cast Sync", (_link->syncCast() ? GFXUI_BUTTON_FLAG_STATE : 0)
  ),
  _btn_msg_send_sync(
    GfxUILayout(
      internalPosX(), (_btn_conf_syncast.elementPosY() + _btn_conf_syncast.elementHeight()),
      130, ((sty.text_size * 8) + 12),  // TODO: Better, but still arbitrary.
      2, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Send Sync", GFXUI_BUTTON_FLAG_MOMENTARY
  ),
  _btn_ses_hangup(
    GfxUILayout(
      internalPosX(), (_btn_msg_send_sync.elementPosY() + _btn_msg_send_sync.elementHeight()),
      130, ((sty.text_size * 8) + 12),  // TODO: Better, but still arbitrary.
      2, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty, "Hangup", GFXUI_BUTTON_FLAG_MOMENTARY
  ),
  _txt(
    GfxUILayout(
      internalPosX(), internalPosY(),
      internalWidth(), (internalHeight() - _tab_bar.elementHeight()),
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty
  )
{
  // Note our subordinate objects...
  if (nullptr != _link->localIdentity()) {
    _content_conf.add_child(
      new GfxUIIdentity(
        GfxUILayout(
          internalPosX(), internalPosY()+_tab_bar.elementHeight(),
          internalWidth(), (internalHeight() - _tab_bar.elementHeight()),
          1, 0, 0, 0,   // Margins_px(t, b, l, r)
          0, 0, 0, 0    // Border_px(t, b, l, r)
        ),
        GfxUIStyle(0, // bg
          0xFFFFFF,   // border
          0xFFFFFF,   // header
          0x40B2AA,   // active
          0xA0A0A0,   // inactive
          0xFFFFFF,   // selected
          0x202020,   // unselected
          1           // t_size
        ),
        _link->localIdentity(),
        (GFXUI_FLAG_FREE_THIS_ELEMENT | GFXUI_FLAG_DRAW_FRAME_L | GFXUI_FLAG_DRAW_FRAME_D)
      )
    );
  }
  _content_info.add_child(&_txt);
  _content_ses.add_child(&_btn_conf_syncast);
  _content_ses.add_child(&_btn_msg_send_sync);
  _content_ses.add_child(&_btn_ses_hangup);

  addTab("Overview",     &_content_info, true);
  addTab("Conf",         &_content_conf);
  addTab("Messages",     &_content_msg);
  addTab("Counterparty", &_content_ses);
}


int GfxUIMLink::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder _tmp_sbldr;
  switch (_tab_bar.activeTab()) {
    case 0:
      _txt.clear();
      _link->printDebug(&_tmp_sbldr);
      _txt.pushBuffer(&_tmp_sbldr);
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      if (_btn_conf_syncast.pressed() ^ _link->syncCast()) {
        _link->syncCast(_btn_conf_syncast.pressed());
        //_btn_conf_syncast.pressed(_link->syncCast());
      }
      if (_btn_msg_send_sync.pressed()) {
      }
      if (_btn_ses_hangup.pressed()) {
        _link->hangup();
      }
      //_tmp_sbldr.concatf(M2MLink::sessionStateStr(_link->getState()));
      break;
    default:
      break;
  }
  return GfxUITabbedContentPane::_render(ui_gfx);
}


bool GfxUIMLink::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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

#endif  // CONFIG_C3P_M2M_SUPPORT
