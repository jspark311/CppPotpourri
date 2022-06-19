/*
File:   GfxUI.cpp
Author: J. Ian Lindsay
Date:   2022.05.29

*/

#include "GfxUI.h"


/*******************************************************************************
* GfxUIElement Base Class
*******************************************************************************/

GfxUIElement::GfxUIElement(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t f) :
  _x(x), _y(y), _w(w), _h(h),
  _mrgn_t(0), _mrgn_b(0), _mrgn_l(0), _mrgn_r(0),
  _flags(f | GFXUI_FLAG_NEED_RERENDER) {}


void GfxUIElement::reposition(uint32_t x, uint32_t y) {
  int32_t shift_x = x - _x;
  int32_t shift_y = y - _y;
  _x += shift_x;
  _y += shift_y;
  if (_children.hasNext()) {
    // There are child objects to relocate.
    const uint QUEUE_SIZE = _children.size();
    for (uint n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      ui_obj->reposition(ui_obj->elementPosX() + shift_x, ui_obj->elementPosY() + shift_y);
    }
  }
  _need_redraw(true);
}


int GfxUIElement::_add_child(GfxUIElement* chld) {
  return _children.insert(chld);
}


bool GfxUIElement::notify(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y) {
  bool ret = false;
  if (includesPoint(x, y)) {
    ret = _notify(GFX_EVNT, x, y);
    if (!ret) {
      ret = _notify_children(GFX_EVNT, x, y);
    }
  }
  return ret;
}

bool GfxUIElement::_notify_children(const GfxUIEvent GFX_EVNT, const uint32_t x, const uint32_t y) {
  if (_children.hasNext()) {
    // There are child objects to notify.
    const uint QUEUE_SIZE = _children.size();
    for (uint n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      if (ui_obj->notify(GFX_EVNT, x, y)) {
        return true;
      }
    }
  }
  return false;
}


int GfxUIElement::render(UIGfxWrapper* ui_gfx, bool force) {
  int ret = 0;
  ret += _render_children(ui_gfx, force);
  if (_need_redraw() | force) {
    ret += _render(ui_gfx);
    if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_U) {
      ui_gfx->img()->drawFastHLine(_x, _y, _w, 0xFFFFFF);
    }
    if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_D) {
      ui_gfx->img()->drawFastHLine(_x, (_y+(_h-1)), _w, 0xFFFFFF);
    }
    if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_L) {
      ui_gfx->img()->drawFastVLine(_x, _y, _h, 0xFFFFFF);
    }
    if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_R) {
      ui_gfx->img()->drawFastVLine((_x+(_w-1)), _y, _h, 0xFFFFFF);
    }
    _need_redraw(false);
  }
  return ret;
};

int GfxUIElement::_render_children(UIGfxWrapper* ui_gfx, bool force) {
  int ret = 0;
  if (_children.hasNext()) {
    // There are child objects to render.
    const uint QUEUE_SIZE = _children.size();
    for (uint n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      ret += ui_obj->render(ui_gfx, force);
    }
  }
  return ret;
}




/*******************************************************************************
* GfxUIButton
*******************************************************************************/

int GfxUIButton::_render(UIGfxWrapper* ui_gfx) {
  uint32_t current_color = elementActive() ? _color_active_on : 0x909090;
  ui_gfx->drawButton(
    _internal_PosX(), _internal_PosY(),
    _internal_Width(), _internal_Height(),
    current_color,
    pressed()
  );
  return 1;
}

bool GfxUIButton::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (!momentary()) {  _class_flip_flag(GFXUI_BUTTON_FLAG_STATE);  }
      else {               _class_set_flag(GFXUI_BUTTON_FLAG_STATE);   }
      ret = true;
      break;
    case GfxUIEvent::RELEASE:
      if (momentary()) {
        _class_clear_flag(GFXUI_BUTTON_FLAG_STATE);
        ret = true;
      }
      break;
    default:
      return false;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


int GfxUITextButton::_render(UIGfxWrapper* ui_gfx) {
  GfxUIButton::_render(ui_gfx);
  uint32_t current_color = elementActive() ? _color_active_on : 0x909090;
  ui_gfx->img()->setCursor(_internal_PosX()+3, _internal_PosY()+3);
  ui_gfx->img()->setTextColor(
    (pressed() ? 0 : current_color),
    (pressed() ? current_color : 0)
  );
  ui_gfx->img()->writeString(_TXT);
  return 1;
}



/*******************************************************************************
* GfxUITabBar
*******************************************************************************/
GfxUITabBar::GfxUITabBar(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, f), _color(color) {}


int GfxUITabBar::_render(UIGfxWrapper* ui_gfx) {
  int8_t ret = 0;
  const uint32_t BTN_COUNT = (uint32_t) _children.size();
  for (uint btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
    GfxUIButton* btn_cur = (GfxUIButton*) _children.get(btn_idx);
    if (btn_cur->pressed()) {
      if (1 == _set_active_tab(btn_idx)) {
        ret = 1;
      }
    }
  }
  return ret;
}


bool GfxUITabBar::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    default:
      return false;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


int8_t GfxUITabBar::addTab(const char* txt, bool selected) {
  int8_t ret = -1;
  const uint32_t flgs_inact = 0;
  const uint32_t flgs_act   = (GFXUI_BUTTON_FLAG_STATE | GFXUI_FLAG_INACTIVE);
  const uint32_t BTN_COUNT = (uint32_t) _children.size() + 1;  // Recalculate width.
  const uint32_t INTRNL_X = _internal_PosX();
  const uint32_t INTRNL_Y = _internal_PosY();
  const uint32_t INTRNL_H = _internal_Height();
  const uint32_t NEW_UNIT_W = (_internal_Width()/BTN_COUNT);

  GfxUIButton* n_btn = new GfxUITextButton(txt,
    INTRNL_X + (NEW_UNIT_W * (BTN_COUNT-1)), INTRNL_Y,
    NEW_UNIT_W, INTRNL_H,
    _color,
    ((GFXUI_FLAG_FREE_THIS_ELEMENT | GFXUI_FLAG_NEED_RERENDER) | (selected ? flgs_act : flgs_inact))
  );
  if (n_btn) {
    ret--;
    if (0 <= _add_child(n_btn)) {
      n_btn->setMargins(0, 2, 0, 0);
      uint32_t x_pix_accum = 0;
      for (uint btn_idx = 0; btn_idx < (BTN_COUNT-1); btn_idx++) {
        GfxUIElement* chld = _children.get(btn_idx);
        chld->reposition(INTRNL_X + x_pix_accum, INTRNL_Y);
        chld->resize(NEW_UNIT_W, INTRNL_H);
        x_pix_accum += NEW_UNIT_W;
      }
      _need_redraw(true);
      ret = 0;
    }
    else {
      delete n_btn;   // Oooops.
    }
  }
  return ret;
}


int8_t GfxUITabBar::_set_active_tab(uint8_t tab_idx) {
  int8_t ret = 0;
  if (_active_tab != tab_idx) {
    ret--;
    GfxUIButton* btn_n_act = (GfxUIButton*) _children.get(tab_idx);
    if (nullptr != btn_n_act) {
      const uint32_t BTN_COUNT = (uint32_t) _children.size();
      for (uint btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
        GfxUIButton* btn_p_act = (GfxUIButton*) _children.get(btn_idx);
        if (tab_idx == btn_idx) {
          btn_p_act->buttonState(true);
          btn_n_act->elementActive(false);   // Don't observe twice-selected tabs.
        }
        else {
          // Set all other tabs inactive, and sensitive.
          btn_p_act->buttonState(false);
          btn_p_act->elementActive(true);
        }
      }
      _active_tab = tab_idx;
      _need_redraw(true);
      ret = 1;
    }
  }
  return ret;
}



/*******************************************************************************
* GfxUISlider
*******************************************************************************/

GfxUISlider::GfxUISlider(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, f), _color_marker(color) {}


int GfxUISlider::_render(UIGfxWrapper* ui_gfx) {
  if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
    ui_gfx->drawProgressBarV(
      _x, _y, _w, _h, _color_marker,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  else {
    ui_gfx->drawProgressBarH(
      _x, _y, _w, _h, _color_marker,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  return 1;
}


bool GfxUISlider::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
        const float PIX_POS_REL = y - _y;
        _percentage = 1.0f - strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float)_h)));
      }
      else {
        const float PIX_POS_REL = x - _x;
        _percentage = strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float)_w)));
      }
    case GfxUIEvent::RELEASE:
      return true;

    case GfxUIEvent::MOVE_UP:
      _percentage = strict_min(1.0, (_percentage + 0.01));
      return true;

    case GfxUIEvent::MOVE_DOWN:
      _percentage = strict_max(0.0, (_percentage - 0.01));
      return true;

    default:
      return false;
  }
}



/*******************************************************************************
* GfxUIKeyValuePair
*******************************************************************************/

GfxUIMagnifier::GfxUIMagnifier(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, f), _color(color)
{
  _scale = 0.5;
}


int GfxUIMagnifier::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}

bool GfxUIMagnifier::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::MOVE_UP:
      _scale = strict_min(1.0, (_scale + 0.01));
      return true;

    case GfxUIEvent::MOVE_DOWN:
      _scale = strict_max(0.01, (_scale - 0.01)); // TODO: Need a more appropriate bounding.
      return true;

    default:
      break;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}



/*******************************************************************************
* GfxUITextArea
*******************************************************************************/

int GfxUITextArea::_render(UIGfxWrapper* ui_gfx) {
  uint next_row   = 0;
  ui_gfx->img()->setTextSize(1);
  ui_gfx->img()->setTextColor(_color_text, 0);  // TODO: Use the correct API when you aren't exhausted.
  uint16_t y_adv = ui_gfx->img()->getFontHeight();
  if (y_adv) _max_rows = _h / y_adv;

  if (0 == _max_cols) {
    // This probably means the object hasn't been rendered yet.
    // NOTE: Assumes monospaced fonts.
    // void getTextBounds(const uint8_t* s, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h);
    // void getTextBounds(StringBuilder*, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h);
    uint16_t x_adv = ui_gfx->img()->getFontWidth();
    if (x_adv) _max_cols = _w / x_adv;
  }

  if ((0 < _max_cols) & (0 < _max_rows)) {
    ui_gfx->img()->fillRect(_x, _y, _w, _h, 0);   // TODO: Use the correct API when you aren't exhausted.
    uint line_count = _scrollback.count();
    uint line_idx   = 0;
    if (line_count > _max_rows) {
      line_idx = (line_count - _max_rows);
      //line_idx = (line_count - _max_rows) - _top_line;
      line_count = _max_rows;
    }

    while (line_count > 0) {
      char* line = _scrollback.position(line_idx);
      if (line) {
        if (strlen(line) > _max_cols) {
          // Shorten the line length to fit the area.
          *(line + _max_cols) = 0;  // TODO: Ugly. Won't go backward.
        }
        ui_gfx->img()->setCursor(_x, _y + (next_row * y_adv));
        ui_gfx->img()->writeString((const char*) line);
        next_row++;
      }
      line_count--;
      line_idx++;
    }
  }
  return 1;
}


bool GfxUITextArea::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::MOVE_UP:
      _top_line = (uint32_t) strict_min((int32_t)(_scrollback.count() - _max_rows), (int32_t)(_top_line + 1));
      return true;

    case GfxUIEvent::MOVE_DOWN:
      _top_line = (uint32_t) strict_max((int32_t) 0, (int32_t) (_top_line - 1));
      return true;

    default:
      return false;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


int8_t GfxUITextArea::provideBuffer(StringBuilder* buf) {
  int8_t ret = 0;
  uint additional_length = buf->length();

  if (0 < additional_length) {   // Reject empty input strings.
    // TODO: By considering the allocated area, and features like text-wrap, I
    //   could make this far cheaper in terms of both mem and cycles.

    if (additional_length >= _max_scrollback_bytes) {
      // If the inbound buffer is itself too large for the scrollback buffer, it
      //   is a given that we can release what we have, and cull the inbound down
      //   to size.
      _scrollback.clear();
      _scrollback.concatHandoff(buf);
      if (additional_length > _max_scrollback_bytes) {
        // Do not exceed defined maximum.
        _scrollback.cull(wrap_accounted_delta(additional_length, _max_scrollback_bytes));
      }
      additional_length = 0;  // We just took all the additional length.
      ret = 1;  // We claimed the entire buffer.
    }
    else {
      // Peak memory load will be greatly reduced if we cull the
      //   scrollback prior to making it longer.
      // Most of the time, we will be here, dropping the oldest scrollback one
      //   token at a time until the new buffer fits.
      while ((additional_length + _scrollback.length()) > _max_scrollback_bytes) {
        _scrollback.drop_position(0);
      }
      buf->split("\n");
      _scrollback.concatHandoff(buf);
    }

    // In all cases, we now need to re-tokenize the buffer on a per-line basis.
    uint lines = _scrollback.count();
    if (!scrollable()) {
      // We may as well drop any lines that will never be viewed again.
      while (lines > _max_rows) {
        _scrollback.drop_position(0);
        lines--;
      }
    }

    _need_redraw(true);
  }
  return ret;
}


/*******************************************************************************
* GfxUI3AxisRender
*******************************************************************************/

int GfxUI3AxisRender::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}


bool GfxUI3AxisRender::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    default:
      return false;
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


/*******************************************************************************
* GfxUISensorFilter
*******************************************************************************/

template <> int GfxUISensorFilter<uint32_t>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (_filter->dirty()) {
    ui_gfx->drawGraph(
      _x, _y, strict_min((uint16_t) _filter->windowSize(), _w), _h, _color,
      true, showRange(), showValue(),
      _filter
    );
    ret++;
  }
  else if (_filter->initialized()) {
    if (!_filter->windowFull()) {
      StringBuilder temp_txt;
      ui_gfx->img()->setCursor(_x + 1, _y + 1);
      ui_gfx->img()->setTextSize(0);
      ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x0000FFFF), 0);
      temp_txt.concatf("%3u / %3u", _filter->lastIndex(), _filter->windowSize());
      ui_gfx->img()->writeString(&temp_txt);
      ret++;
    }
  }
  else {
    ui_gfx->img()->setCursor(_x + 1, _y + 1);
    ui_gfx->img()->setTextSize(0);
    ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x000000FF));
    ui_gfx->img()->writeString("Not init'd");
  }
  return ret;
}


template <> bool GfxUISensorFilter<uint32_t>::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
    case GfxUIEvent::RELEASE:
      showValue(GfxUIEvent::TOUCH == GFX_EVNT);
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


template <> int GfxUISensorFilter<float>::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  if (_filter->dirty()) {
    ui_gfx->drawGraph(
      _x, _y, strict_min((uint16_t) _filter->windowSize(), _w), _h, _color,
      true, showRange(), showValue(),
      _filter
    );
    ret++;
  }
  else if (_filter->initialized()) {
    if (!_filter->windowFull()) {
      StringBuilder temp_txt;
      ui_gfx->img()->setCursor(_x + 1, _y + 1);
      ui_gfx->img()->setTextSize(0);
      ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x0000FFFF), 0);
      temp_txt.concatf("%3u / %3u", _filter->lastIndex(), _filter->windowSize());
      ui_gfx->img()->writeString(&temp_txt);
      ret++;
    }
  }
  else {
    ui_gfx->img()->setCursor(_x + 1, _y + 1);
    ui_gfx->img()->setTextSize(0);
    ui_gfx->img()->setTextColor(ui_gfx->img()->convertColor(0x000000FF));
    ui_gfx->img()->writeString("Not init'd");
  }
  return ret;
}


template <> bool GfxUISensorFilter<float>::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
    case GfxUIEvent::RELEASE:
      showValue(GfxUIEvent::TOUCH == GFX_EVNT);
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
int GfxUIKeyValuePair::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}

bool GfxUIKeyValuePair::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
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
