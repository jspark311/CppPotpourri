/*
File:   GfxUIKeyValuePair.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"
#include "../ImageUtils/BlobPlotter.h"


/*
* Given a TCode, formulate a provided style object to give the type a uniform
*   visual representation. Returns the same pointer that was provided to allow
*   easy pass-through during construction.
*/
static GfxUIStyle* gfxui_style_for_tcode(const TCode TC, GfxUIStyle* s) {
  if (nullptr != s) {
    s->color_bg     = 0x000000;
    s->color_border = 0x000000;
    uint32_t    acc = 0x000000;

    switch (TC) {
      // Numerics
      case TCode::INT8:    acc += 0x191900;
      case TCode::INT16:   acc += 0x191900;
      case TCode::INT32:   acc += 0x191900;
      case TCode::INT64:   acc += 0x191900;
        acc += 0x5072ff;
        break;
      case TCode::UINT8:   acc += 0x191900;
      case TCode::UINT16:  acc += 0x191900;
      case TCode::UINT32:  acc += 0x191900;
      case TCode::UINT64:  acc += 0x191900;
        acc += 0x4d93ff;
        break;
      case TCode::FLOAT:   acc += 0x2a0c0c;
      case TCode::DOUBLE:  acc += 0x2a0c0c;
        acc += 0x70dbdb;
        break;
      case TCode::BOOLEAN:       acc += 0xb3b3ff;
        break;
      // Strings and aliases
      case TCode::STR:           acc += 0x000818;
      case TCode::STR_BUILDER:   acc += 0x000818;
      case TCode::SI_UNIT:       acc += 0x000818;
        acc += 0xff9933;
        break;
      // Vectors
      case TCode::VECT_3_INT8:   acc += 0x001800;
      case TCode::VECT_3_INT16:  acc += 0x001800;
      case TCode::VECT_3_INT32:  acc += 0x001800;
        acc += 0xff4dff;
        break;
      case TCode::VECT_3_UINT8:  acc += 0x0811900;
      case TCode::VECT_3_UINT16: acc += 0x0811900;
      case TCode::VECT_3_UINT32: acc += 0x0811900;
        acc += 0xd580ff;
        break;
      case TCode::VECT_3_FLOAT:  acc += 0x002a0d;
      case TCode::VECT_3_DOUBLE: acc += 0x002a0d;
        acc += 0xff80bf;
        break;
      // Non-convertible types
      case TCode::IMAGE:          acc += 0x4dff4d;        break;
      case TCode::KVP:            acc += 0x00e673;        break;
      case TCode::IDENTITY:       acc += 0x00e6ac;        break;
      case TCode::STOPWATCH:      acc += 0x9fdfbf;        break;

      // Pointer-length compounds, and their aliases.
      case TCode::BINARY:         acc += 0xbbbb77;
        break;
      default:
        break;
    }
    s->color_active = acc;
  }
  return s;
}




/*******************************************************************************
* GfxUIC3PType
*******************************************************************************/
GfxUIC3PType::GfxUIC3PType(const TCode TC, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _type((C3PType*) getTypeHelper(TC))
{
  gfxui_style_for_tcode(TC, &_style);
};


int GfxUIC3PType::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (nullptr != _type) {
    uint32_t i_x = internalPosX();
    uint32_t i_y = internalPosY();
    // uint16_t i_w = internalWidth();
    // uint16_t i_h = internalHeight();
    //ui_gfx->img()->fillRect(i_x, i_y, i_w, i_h, _style.color_bg);   // TODO: Somehow this is out of bounds.
    ui_gfx->img()->setCursor(i_x, i_y);
    ui_gfx->img()->setTextSize(_style.text_size);
    ui_gfx->img()->setTextColor(_style.color_active);
    ui_gfx->img()->writeString(typecodeToStr(_type->TCODE));
    ret = 1;
  }
  return ret;
}


bool GfxUIC3PType::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  return false;
}



/*******************************************************************************
* GfxUIC3PValue
*******************************************************************************/
GfxUIC3PValue::GfxUIC3PValue(C3PValue* value, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _value(value), _last_trace(0),
  _stacked_ir(inhibitRefresh()), _stacked_sti(showTypeInfo()) {}


GfxUIC3PValue::~GfxUIC3PValue() {
  inhibitRefresh(true);
  GfxUIElement* tmp_gfxui = _children.dequeue();
  if (nullptr != tmp_gfxui) {
    delete tmp_gfxui;
  }
  if (reapObject() & (nullptr != _value)) {
    delete _value;
  }
  _value = nullptr;
}


int GfxUIC3PValue::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (!inhibitRefresh() & (nullptr != _value)) {
    const bool SHOW_TYPE_INFO = ((isFocused() & hoverResponse()) | showTypeInfo());
    uint32_t i_x = internalPosX();
    uint32_t i_y = internalPosY();
    uint16_t i_w = internalWidth();
    uint16_t i_h = internalHeight();
    bool have_type_obj  = (0 < _children.size());
    bool flip_type_obj  = (have_type_obj ^ showTypeInfo());
    if (flip_type_obj) {
      if (have_type_obj) {
        GfxUIElement* tmp_gfxui = _children.dequeue();
        if (nullptr != tmp_gfxui) {
          delete tmp_gfxui;
        }
      }
      else {
        // Automatically set the width to the minimum required.
        const char* TC_STRING = typecodeToStr(_value->tcode());
        const uint8_t TC_STR_PAD_PX_MIN = 68;
        uint32_t type_x = 0;
        uint32_t type_y = 0;
        uint32_t type_w = 0;
        uint32_t type_h = 0;
        ui_gfx->img()->getTextBounds((const uint8_t*) TC_STRING, i_x, i_y, &type_x, &type_y, &type_w, &type_h);
        GfxUILayout tc_layout(
          i_x, i_y, TC_STR_PAD_PX_MIN, type_h,
          0, 0, 0, (TC_STR_PAD_PX_MIN - type_w),
          0, 0, 0, 0
        );
        GfxUIStyle  tc_style(_style);
        GfxUIC3PType* gfxui_type = new GfxUIC3PType(_value->tcode(), &tc_layout, _style);
        if (nullptr != gfxui_type) {
          _add_child(gfxui_type);
          have_type_obj = true;
        }
      }
      ret = 1;
    }

    if (_value->dirty(&_last_trace)) {
      if (SHOW_TYPE_INFO & have_type_obj) {
        // Offset the X-pos to avoid clobbering the type render.
        i_x += _children.get(0)->elementWidth();
      }

      ui_gfx->img()->fillRect(i_x, i_y, i_w, i_h, _style.color_bg);
      StringBuilder line;
      if (_value->is_ptr_len()) {
        // Graphical environments can handle doing things a bit smarter than
        //   just string-dumping.
        BlobStylerHeatMap blob_style(ui_gfx->img(), 0, 0);
        BlobPlotterHilbertCurve curve_render(
          &blob_style, _value, ui_gfx->img(),
          i_x, i_y, i_w, i_h
        );
        int8_t ret_local = curve_render.apply();
        if (0 != ret_local) {
          line.concatf("%u bytes", _value->length());
          ui_gfx->img()->setCursor(i_x, i_y);
          ui_gfx->img()->setTextSize(_style.text_size);
          ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
          ui_gfx->img()->writeString(&line);
        }
        ret = 1;
      }
      else {
        // Unspecialized workflows (generally, anything that can sensibly be
        //   rendered to a string).
        _value->toString(&line, false);
        //if (line.length() > some_limit) {
        //}
        ui_gfx->img()->setCursor(i_x, i_y);
        ui_gfx->img()->setTextSize(_style.text_size);
        ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
        ui_gfx->img()->writeString(&line);
        ret = 1;
      }
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
