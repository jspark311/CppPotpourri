/*
File:   GfxUIKeyValuePair.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUIC3PType
*******************************************************************************/
GfxUIC3PType::GfxUIC3PType(const TCode TC, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _type((C3PType*) getTypeHelper(TC)) {};


int GfxUIC3PType::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (nullptr != _type) {
    uint32_t i_x = internalPosX();
    uint32_t i_y = internalPosY();
    uint16_t i_w = internalWidth();
    StringBuilder line(typecodeToStr(_type->TCODE));
    ui_gfx->img()->setCursor(i_x, i_y);
    ui_gfx->img()->setTextSize(_style.text_size);
    ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
    ui_gfx->img()->writeString(&line);
    ret = 1;
  }
  return ret;
}


bool GfxUIC3PType::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
* GfxUIC3PValue
*******************************************************************************/
GfxUIC3PValue::GfxUIC3PValue(C3PValue* value, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  //_type_render(
  //  ((nullptr != value) ? value->TCODE : TCode::NONE),
  //  GfxUILayout(
  //    internalPosX() + (internalWidth() - 55), internalPosY(),
  //    55, (sty.text_size * 8),  // TODO: Better, but still arbitrary.
  //    0, 0, 0, 0,              // Margin_px(t, b, l, r)
  //    0, 0, 0, 0               // Border_px(t, b, l, r)
  //  ),
  //  sty
  //),
  _value(value), _last_trace(0),
  _stacked_ir(inhibitRefresh()), _stacked_sti(showTypeInfo())
{
  // TODO: Make a deep-copy?
  //_add_child(&_type_render);
}


GfxUIC3PValue::~GfxUIC3PValue() {
  if (reapObject() & (nullptr != _value)) {
    delete _value;
  }
  _value = nullptr;
}


int GfxUIC3PValue::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (!inhibitRefresh()) {
    if (_value->dirty(&_last_trace)) {
      uint32_t i_x = internalPosX();
      uint32_t i_y = internalPosY();
      uint16_t i_w = internalWidth();
      uint16_t i_h = internalHeight();
      ui_gfx->img()->fillRect(i_x, i_y, i_w, i_h, _style.color_bg);

      const bool SHOW_TYPE_INFO = ((isFocused() & hoverResponse()) | showTypeInfo());
      StringBuilder line;
      _value->toString(&line, SHOW_TYPE_INFO);
      ui_gfx->img()->setCursor(i_x, i_y);
      ui_gfx->img()->setTextSize(_style.text_size);
      ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
      ui_gfx->img()->writeString(&line);
      ret = 1;
    }
  }
  return ret;
}


bool GfxUIC3PValue::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      _stacked_ir = !_stacked_ir;
      if (!hoverResponse()) {
        inhibitRefresh(_stacked_ir);
      }
      ret = true;
      break;

    case GfxUIEvent::DRAG_START:
      //reposition(x, y);
      ret = true;
      break;

    default:
      break;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}



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
