/*
File:   GfxUITextArea.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUITextArea
*******************************************************************************/

int GfxUITextArea::_render(UIGfxWrapper* ui_gfx) {
  uint32_t next_row   = 0;
  uint32_t i_x = internalPosX();
  uint32_t i_y = internalPosY();
  uint16_t i_w = internalWidth();
  uint16_t i_h = internalHeight();
  ui_gfx->img()->setTextSize(_style.text_size);
  ui_gfx->img()->setTextColor(_style.color_active, _style.color_bg);
  uint16_t y_adv = ui_gfx->img()->getFontHeight();
  if (y_adv) _max_rows = i_h / y_adv;

  if (0 == _max_cols) {
    // This probably means the object hasn't been rendered yet.
    // NOTE: Code below assumes monospaced fonts. Use the commented code if
    //   this assumption fails one day.
    // void getTextBounds(const uint8_t* s, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h);
    // void getTextBounds(StringBuilder*, uint32_t x, uint32_t y, uint32_t* x1, uint32_t* y1, uint32_t* w, uint32_t* h);
    uint16_t x_adv = ui_gfx->img()->getFontWidth();
    if (x_adv) _max_cols = i_w / x_adv;
  }

  if ((0 < _max_cols) & (0 < _max_rows)) {
    ui_gfx->img()->fillRect(i_x, i_y, i_w, i_h, _style.color_bg);
    uint16_t line_count = _scrollback.count();
    uint16_t line_idx   = 0;
    if (line_count > _max_rows) {
      if (scrollable()) {
        line_idx   = (line_count - _max_rows) - _top_line;
        line_count = strict_min((uint16_t) (line_count-line_idx), _max_rows);
      }
      else {
        // If the TextArea is locked, only render the bottom of the buffer.
        line_idx = (line_count - _max_rows);
        line_count = _max_rows;
      }
    }

    while (line_count > 0) {
      char* line = _scrollback.position(line_idx);
      if (line) {
        if (strlen(line) > _max_cols) {
          // Shorten the line length to fit the area.
          *(line + _max_cols) = 0;  // TODO: Ugly. Won't go backward.
        }
        ui_gfx->img()->setCursor(i_x, i_y + (next_row * y_adv));
        ui_gfx->img()->writeString((const char*) line);
        next_row++;
      }
      line_count--;
      line_idx++;
    }
  }
  return 1;
}


bool GfxUITextArea::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::MOVE_UP:
      if (scrollable()) {
        //_top_line = (uint32_t) strict_min((int32_t)(_scrollback.count() - _max_rows), (int32_t)(_top_line + 1));
        if (_top_line < (_scrollback.count() - _max_rows)) {
          _top_line++;
          //_top_line = strict_range_bind(_top_line, 0, (_max_rows - _top_line));
          ret = true;
        }
      }
      break;

    case GfxUIEvent::MOVE_DOWN:
      if (scrollable()) {
        //_top_line = (uint32_t) strict_max((int32_t) 0, (int32_t) (_top_line - 1));
        if (_top_line > 0) {
          _top_line--;
          //_top_line = strict_range_bind(_top_line, 0, (_max_rows - _top_line));
          ret = true;
        }
      }
      break;

    default:
      break;
  }

  if (ret) {
    _need_redraw(true);
  }
  return ret;
}


int8_t GfxUITextArea::pushBuffer(StringBuilder* buf) {
  int8_t ret = 0;
  uint32_t additional_length = buf->length();

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
        _scrollback.cull(strict_abs_delta(additional_length, _max_scrollback_bytes));
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
      _scrollback.concatHandoff(buf);
      _scrollback.string();
      buf->split("\n");
    }

    // In all cases, we now need to re-tokenize the buffer on a per-line basis.
    uint32_t lines = _scrollback.count();
    if (scrollable()) {
      // If the desired top_line is such that the last line of the text
      //   was visible before the append, update it again to maintain that
      //   condition.
      // The net result ought to be that scroll locks to the bottom of output if
      //   it was already there, but scroll ceases under additional input
      //   otherwise (up to the buffering limits).
      // TODO: Not quite there....
      //_top_line = strict_range_bind(lines, 0, (_max_rows - lines));
    }
    else {
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


/**
*
* @return the number of bytes available.
*/
int32_t GfxUITextArea::bufferAvailable() {
  int32_t ret = 2048;  // TODO: Find sensible value.
  return ret;
}


void GfxUITextArea::clear() {
  _scrollback.clear();
  if (scrollable()) {
    _top_line = 0;
  }
  _need_redraw(true);
}
