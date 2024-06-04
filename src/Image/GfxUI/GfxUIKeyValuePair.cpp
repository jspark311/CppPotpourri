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
  _kvp(kvp),
  _bounding_w(0), _bounding_h(0),
  _kvp_loaded(false) {};


int8_t GfxUIKeyValuePair::showKVP(KeyValuePair* new_kvp) {
  int8_t ret = 0;
  _kvp = new_kvp;  // TODO: Probably not sufficient...
  _need_redraw(true);
  return ret;
}


/*
* This function does a calculation to determine how large the render will need
*   to be to fully display the current KVP. This function is unconstrained by
*   what dimentions are achievable or desirable. That control must be applied
*   elsewhere.
* TODO: How to keep a very large KVP from wrecking the GUI? Hard-bounded for now.
*/
//int8_t GfxUIKeyValuePair::_calculate_bounding_box(PixUInt* w, PixUInt* h) {
//  int8_t ret = -1;
//  if (nullptr != _kvp) {
//    // Set text size first so that text size calculations are valid. But save the
//    //   existing state. We aren't certain if we are rendering or not.
//    const uint8_t ORIGINAL_TXT_SZ = ui_gfx->img()->getTextSize();
//    ui_gfx->img()->setTextSize(_style.text_size);
//    const PixUInt FONT_WIDTH  = ui_gfx->img()->getFontWidth();
//    const PixUInt FONT_HEIGHT = ui_gfx->img()->getFontHeight();
//    ui_gfx->img()->setTextSize(ORIGINAL_TXT_SZ);  // Restore the setting we found.
//
//    if (nullptr != w) {  *w = 0;  }
//    if (nullptr != h) {  *h = 0;  }
//    ret = 0;
//  }
//  return ret;
//}



int GfxUIKeyValuePair::_render(UIGfxWrapper* ui_gfx) {
  int8_t ret = 0;
  if (!_kvp_loaded & (nullptr != _kvp)) {
    const PixUInt i_x = internalPosX();
    const PixUInt i_y = internalPosY();
    const PixUInt i_w = internalWidth();
    const PixUInt i_h = internalHeight();
    const uint32_t GFXELEMENT_FLAGSET = ( \
      GFXUI_FLAG_FREE_THIS_ELEMENT | GFXUI_FLAG_ALWAYS_REDRAW | GFXUI_C3PVAL_FLAG_RESIZE_ON_RENDER | \
      (showTypeInfo() ? GFXUI_C3PVAL_FLAG_SHOW_TYPE_INFO : 0));

    // Set text size first so that text size calculations are valid.
    ui_gfx->img()->setTextSize(_style.text_size);
    // Get the list of keys.
    StringBuilder key_list;
    _kvp->collectKeys(&key_list);
    const int MAX_STR_LEN = (key_list.maximumFragmentLength() + 4);  // We will add 4 characters to the key string.
    const PixUInt FONT_WIDTH  = ui_gfx->img()->getFontWidth();
    const PixUInt FONT_HEIGHT = ui_gfx->img()->getFontHeight();
    const PixUInt MAX_KEY_WIDTH = (MAX_STR_LEN * FONT_WIDTH);
    const PixUInt HARDCODED_HEIGHT = (FONT_HEIGHT + 2);   // TODO: Flows...
    PixUInt tracked_x = MAX_KEY_WIDTH;
    PixUInt tracked_y = 0;
    PixUInt max_value_width = 0;

    KeyValuePair* current_kvp = _kvp;
    GfxUIStyle  val_style(_style);
    // TODO: Don't render yet. Might have multi-line values.
    while ((nullptr != current_kvp) & (key_list.count() > 0)) {
      char* cur_key = key_list.position_trimmed(0);
      current_kvp   = _kvp->valueWithKey(cur_key);
      StringBuilder keystr;
      keystr.concatf("\"%s\":", cur_key);

      ui_gfx->img()->setCursor(i_x, (i_y + tracked_y));
      ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
      ui_gfx->img()->writeString(&keystr);

      if (TCode::KVP != current_kvp->tcode()) {
        GfxUILayout val_layout(
          (tracked_x + i_x), (tracked_y + i_y), (i_w - tracked_x), HARDCODED_HEIGHT,
          0, 0, 0, 0,
          0, 0, 0, 0
        );
        GfxUIC3PValue* gfxui_val = new GfxUIC3PValue(
          current_kvp, val_layout, val_style,
          (GFXELEMENT_FLAGSET)
        );
        if (nullptr != gfxui_val) {
          _add_child(gfxui_val);
          tracked_y += HARDCODED_HEIGHT;
        }
      }
      else {
        KeyValuePair* nested_kvp = nullptr;
        if (0 == current_kvp->get_as(&nested_kvp)) {
          const PixUInt VALUE_SUBHEIGHT = (HARDCODED_HEIGHT * nested_kvp->count());

          ui_gfx->img()->setCursor((tracked_x + i_x), (tracked_y + i_y));
          ui_gfx->img()->setTextColor(_style.color_inactive, _style.color_bg);
          ui_gfx->img()->writeString("{");
          tracked_y += HARDCODED_HEIGHT;

          GfxUILayout val_layout(
            (tracked_x + i_x + (FONT_WIDTH * 2)), (tracked_y + i_y), (i_w - tracked_x), VALUE_SUBHEIGHT,
            0, 0, 0, 0,
            0, 0, 0, 0
          );
          GfxUIKeyValuePair* gfxui_val = new GfxUIKeyValuePair(nested_kvp, val_layout, val_style, GFXELEMENT_FLAGSET);
          //GfxUIC3PValue* gfxui_val = new GfxUIC3PValue(nested_kvp, val_layout, val_style, GFXELEMENT_FLAGSET);
          if (nullptr != gfxui_val) {
            _add_child(gfxui_val);
            tracked_y += VALUE_SUBHEIGHT;
          }
          ui_gfx->img()->setCursor((tracked_x + i_x), (tracked_y + i_y));
          ui_gfx->img()->setTextColor(_style.color_inactive, _style.color_bg);
          ui_gfx->img()->writeString("}");
          tracked_y += HARDCODED_HEIGHT;
        }
      }
      key_list.drop_position(0);
    }
    const PixUInt KEY_AREA_HEIGHT = tracked_y;  // The size of the key area is now known.

    _bounding_w = (tracked_x + max_value_width);
    _bounding_h = strict_min(tracked_y, KEY_AREA_HEIGHT);
    if (resizeOnRender()) {
      resize(_bounding_w, _bounding_h);
    }
    _kvp_loaded = true;
    ret = 1;
  }

  if (_kvp_loaded) {
    ret = 1;
  }

  return ret;
}


bool GfxUIKeyValuePair::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
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


bool GfxUIKVPUtil::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
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
