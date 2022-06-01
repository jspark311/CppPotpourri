/*
File:   GfxUI.cpp
Author: J. Ian Lindsay
Date:   2022.05.29

*/

#include "GfxUI.h"


/*******************************************************************************
* GfxUIElement Base Class
*******************************************************************************/

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
    // There are child obects to notify.
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
    _need_redraw(false);
  }
  return ret;
};

int GfxUIElement::_render_children(UIGfxWrapper* ui_gfx, bool force) {
  int ret = 0;
  if (_children.hasNext()) {
    // There are child obects to render.
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
  ui_gfx->drawButton(_x, _y, _w, _h, _color_active_on, pressed());
  return 1;
}

bool GfxUIButton::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      if (momentary()) {  _class_flip_flag(GFXUI_BUTTON_FLAG_STATE);  }
      else {              _class_set_flag(GFXUI_BUTTON_FLAG_STATE);   }
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



/*******************************************************************************
* GfxUITabBar
*******************************************************************************/

int GfxUITabBar::_render(UIGfxWrapper* ui_gfx) {
  return 1;
}

bool GfxUITabBar::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      ret = true;
      break;
    default:
      return false;
  }
  if (ret) {
    _need_redraw(true);
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
      line_idx = line_count - _max_rows;
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
* GfxUIMLink
*******************************************************************************/
#if defined(CONFIG_MANUVR_M2M_SUPPORT)

int GfxUIMLink::_render(UIGfxWrapper* ui_gfx) {
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
