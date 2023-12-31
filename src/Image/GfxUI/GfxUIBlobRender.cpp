/*
File:   GfxUIBlobRender.cpp
Author: J. Ian Lindsay
Date:   2023.12.25

*/

#include "../GfxUI.h"
#include "../ImageUtils/BlobPlotter.h"

/*******************************************************************************
* GfxUIBlobRender
*******************************************************************************/


GfxUIBlobRender::GfxUIBlobRender(C3PValue* value, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _value(value),
  _plotter(nullptr),
  _styler(nullptr),
  _plotter_id(BlobPlotterID::NONE),
  _styler_id(BlobStylerID::NONE),
  _last_trace(0),
  _stacked_ir(inhibitRefresh()),

  _plotter_selector(
    GfxUILayout(
      (internalPosX()+(internalWidth()-160)), internalPosY(),
      160, ((sty.text_size * 8) + 16),  // TODO: Better, but still arbitrary.
      1, 1, 1, 0,
      0, 0, 0, 0               // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      0xcF10c0,   // active
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      sty.text_size  // t_size
    ),
    (GFXUI_TABBAR_FLAG_SCROLL_CYCLES_TABS)
  ),

  _style_selector(
    GfxUILayout(
      _plotter_selector.elementPosX(), (_plotter_selector.elementPosY() + _plotter_selector.elementHeight()),
      _plotter_selector.elementWidth(), ((sty.text_size * 8) + 16),  // TODO: Better, but still arbitrary.
      1, 1, 1, 0,
      0, 0, 0, 0               // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      0xdf20d0,   // active
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      sty.text_size  // t_size
    ),
    (GFXUI_TABBAR_FLAG_SCROLL_CYCLES_TABS)
  ),

  _hex_txt(
    GfxUILayout(
      _style_selector.elementPosX(), (_style_selector.elementPosY() + _style_selector.elementHeight()),
      _plotter_selector.elementWidth(), 64,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      0xa0a000,   // active
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      1  // t_size
    ),
    (0)
  ),

  _map_txt(
    GfxUILayout(
      _hex_txt.elementPosX(), (_hex_txt.elementPosY() + _hex_txt.elementHeight()),
      _plotter_selector.elementWidth(), 64,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    GfxUIStyle(0, // bg
      0xFFFFFF,   // border
      0xFFFFFF,   // header
      0xb0b010,   // active
      0xA0A0A0,   // inactive
      0xFFFFFF,   // selected
      0x202020,   // unselected
      1  // t_size
    ),
    (0)
  )
{
  // Note our subordinate objects...
  _plotter_selector.addTab("Linear");
  _plotter_selector.addTab("Hilbert", true);
  _style_selector.addTab("Heat", true);
  _style_selector.addTab("Entropy");
  _style_selector.addTab("MAP File");
  _add_child(&_plotter_selector);
  _add_child(&_style_selector);
  _add_child(&_hex_txt);
  _add_child(&_map_txt);
}


GfxUIBlobRender::~GfxUIBlobRender() {
  inhibitRefresh(true);
  GfxUIElement* tmp_gfxui = _children.dequeue();
  if (nullptr != tmp_gfxui) {
    delete tmp_gfxui;
  }
}


// TODO: Can't use a "path" here, nor a File. It must be an abstracted structure
//   of some sort.
int8_t GfxUIBlobRender::setMapFile(char* p) {
  int8_t ret = -1;
  return ret;
}



int8_t GfxUIBlobRender::_check_plotter(Image* img) {
  int8_t ret = -1;
  BlobPlotterID desired = BlobPlotterID::INVALID;
  switch (_plotter_selector.activeTab()) {
    case 0:    desired = BlobPlotterID::LINEAR;     break;
    case 1:    desired = BlobPlotterID::HILBERT;    break;
    default:   return ret;
  }
  ret--;
  bool need_to_create = false;

  if (nullptr != _plotter) {                 // If there already exists a plotter...
    if (desired == _plotter->plotterID()) {  // ...and it is of the kind desired...
      ret = 0;                               // ...take no action and return success.
    }
    else {                                   // If it isn't what is desired...
      delete _plotter;                       // ...delete the old one.
      _plotter = nullptr;
      need_to_create = true;
    }
  }
  else {
    need_to_create = true;
  }

  if (need_to_create) {   // If a plotter needs to be created, do so.
    uint32_t i_x = internalPosX();
    uint32_t i_y = internalPosY();
    uint16_t i_w = (internalWidth() - 160);
    uint16_t i_h = internalHeight();
    switch (desired) {
      case BlobPlotterID::LINEAR:
        _plotter = new BlobPlotterLinear(
          nullptr, _value, img,
          i_x, i_y, i_w, i_h
        );
        break;
      case BlobPlotterID::HILBERT:
        _plotter = new BlobPlotterHilbertCurve(
          nullptr, _value, img,
          i_x, i_y, i_w, i_h
        );
        break;
      default:   return ret;
    }
    if (nullptr != _plotter) {
      ret = 1;   // Configure it if successful.
    }
  }
  return ret;
}



int8_t GfxUIBlobRender::_check_styler(Image* img) {
  int8_t ret = -1;
  BlobStylerID desired = BlobStylerID::INVALID;
  switch (_style_selector.activeTab()) {
    case 0:    desired = BlobStylerID::HEAT;       break;
    case 1:    desired = BlobStylerID::ENTROPY;    break;
    case 2:    desired = BlobStylerID::FENCING;    break;
    default:   return ret;
  }
  ret--;
  bool need_to_create = false;

  if (nullptr != _styler) {                  // If there already exists a styler...
    if (desired == _styler->stylerID()) {    // ...and it is of the kind desired...
      ret = 0;                               // ...take no action and return success.
    }
    else {                                   // If it isn't what is desired...
      delete _styler;                        // ...delete the old one.
      _styler = nullptr;
      need_to_create = true;
    }
  }
  else {
    need_to_create = true;
  }

  if (need_to_create) {   // If a styler needs to be created, do so.
    switch (desired) {
      case BlobStylerID::HEAT:
        _styler = new BlobStylerHeatMap(img, 0, 0);
        break;
      case BlobStylerID::ENTROPY:
        _styler = new BlobStylerEntropyMap(img);
        break;
      case BlobStylerID::FENCING:
        _styler = new BlobStylerExplicitFencing(img);
        // TODO: Add fences from MAP data.
        break;
      default:   return ret;
    }
    if (nullptr != _styler) {
      ret = 1;   // Configure it if successful.
    }
  }
  return ret;
}



/*******************************************************************************
* GfxUIElement implementation
*******************************************************************************/

int GfxUIBlobRender::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (!inhibitRefresh() & (nullptr != _value)) {
    Image* img = ui_gfx->img();
    int8_t p_check = _check_plotter(img);
    int8_t s_check = _check_styler(img);
    bool should_refresh = (p_check > 0) | (s_check > 0);
    if (_value->dirty(&_last_trace) | should_refresh) {
      uint32_t i_x = internalPosX();
      uint32_t i_y = internalPosY();
      uint16_t i_w = internalWidth();
      uint16_t i_h = internalHeight();
      img->fillRect(i_x, i_y, i_w, i_h, _style.color_bg);   // Wipe the field.
      StringBuilder line;
      if (_value->is_ptr_len()) {
        if ((0 == p_check) & (0 == s_check)) {
          _plotter->setStyler(_styler);
          int8_t ret_local = _plotter->apply(true);
          if (0 != ret_local) {
            line.concatf("%u bytes", _value->length());
            img->setCursor(i_x, i_y);
            img->setTextSize(_style.text_size);
            img->setTextColor(_style.color_active, _style.color_bg);
            img->writeString(&line);
          }
          ret = 1;
        }
      }
      else {
        line.concat("Refusing to render data that is not ptr/len.");
        img->setCursor(i_x, i_y);
        img->setTextSize(_style.text_size);
        img->setTextColor(_style.color_active, _style.color_bg);
        img->writeString(&line);
        ret = 1;
      }
    }
  }
  return ret;
}


bool GfxUIBlobRender::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    // case GfxUIEvent::TOUCH:
    //   ret = true;
    //   break;

    // case GfxUIEvent::DRAG_START:
    //   ret = true;
    //   break;

    default:
      break;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}
