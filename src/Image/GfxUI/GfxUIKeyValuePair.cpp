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
  _kvp(kvp), _kvp_loaded(false)
{
  // TODO: Make a deep-copy?
};


int8_t GfxUIKeyValuePair::showKVP(KeyValuePair* new_kvp) {
  int8_t ret = -1;


  return ret;
}


int GfxUIKeyValuePair::_render(UIGfxWrapper* ui_gfx) {
  int8_t ret = 0;

  uint32_t i_x = internalPosX();
  uint32_t i_y = internalPosY();
  uint16_t i_w = internalWidth();
  uint16_t i_h = internalHeight();

  if (!_kvp_loaded & (nullptr != _kvp)) {
    uint32_t tracked_x = i_x;
    uint32_t tracked_y = i_y;

    KeyValuePair* current_kvp = _kvp;
    const uint16_t HARDCODED_HEIGHT = 16;   // TODO: Flows...

    while (nullptr != current_kvp) {
      StringBuilder keystr;
      keystr.concatf("\"%s\":", current_kvp->getKey());
      uint32_t keystr_x = 0;
      uint32_t keystr_y = 0;
      uint32_t keystr_w = 0;
      uint32_t keystr_h = 0;
      ui_gfx->img()->getTextBounds((const uint8_t*) keystr.string(), 0, 0, &keystr_x, &keystr_y, &keystr_w, &keystr_h);
      tracked_x = strict_max(tracked_x, (i_x + keystr_w));
      ui_gfx->img()->setCursor(i_x, tracked_y);
      ui_gfx->img()->setTextSize(_style.text_size);
      ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
      ui_gfx->img()->writeString(&keystr);
      current_kvp = current_kvp->retrieveByIdx(1);
      tracked_y += HARDCODED_HEIGHT;
    }
    current_kvp = _kvp;
    tracked_y = i_y;

    while (nullptr != current_kvp) {
      C3PValue* tmp_val = current_kvp->getValue();
      if (nullptr != tmp_val) {
        if (TCode::KVP != tmp_val->tcode()) {
          GfxUILayout val_layout(
            tracked_x, tracked_y, (i_w-(tracked_x - i_x)), HARDCODED_HEIGHT,
            0, 0, 0, 0,
            0, 0, 0, 0
          );
          GfxUIStyle  val_style(_style);
          val_style.color_active = 0xC0C0C0;
          GfxUIC3PValue* gfxui_val = new GfxUIC3PValue(tmp_val, &val_layout, val_style, (GFXUI_C3PVAL_FLAG_SHOW_TYPE_INFO));
          if (nullptr != gfxui_val) {
            _add_child(gfxui_val);
          }
        }
      }

      current_kvp = current_kvp->retrieveByIdx(1);
      tracked_y += HARDCODED_HEIGHT;
    }
    _kvp_loaded = true;
    ret = 1;
  }

  if (_kvp_loaded) {
    ret = 1;
  }

  return ret;
}


bool GfxUIKeyValuePair::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = true;
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
