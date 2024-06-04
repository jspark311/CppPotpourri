/*
File:   GfxUI.cpp
Author: J. Ian Lindsay
Date:   2022.05.29

This file contains the base implementation of GfxUIElement. All classes in the
  GfxUI feature block ultimately come down to this.
*/

#include "../GfxUI.h"

/*******************************************************************************
* Const enum support
*******************************************************************************/

const EnumDef<GfxUIEvent> _ENUM_LIST[] = {
  { GfxUIEvent::NONE,          "NONE"},
  { GfxUIEvent::TOUCH,         "TOUCH"},
  { GfxUIEvent::RELEASE,       "RELEASE"},
  { GfxUIEvent::PRESSURE,      "PRESSURE"},
  { GfxUIEvent::DRAG,          "DRAG"},
  { GfxUIEvent::HOVER_IN,      "HOVER_IN"},
  { GfxUIEvent::HOVER_OUT,     "HOVER_OUT"},
  { GfxUIEvent::SELECT,        "SELECT"},
  { GfxUIEvent::UNSELECT,      "UNSELECT"},
  { GfxUIEvent::MOVE_UP,       "MOVE_UP"},
  { GfxUIEvent::MOVE_DOWN,     "MOVE_DOWN"},
  { GfxUIEvent::MOVE_LEFT,     "MOVE_LEFT"},
  { GfxUIEvent::MOVE_RIGHT,    "MOVE_RIGHT"},
  { GfxUIEvent::MOVE_IN,       "MOVE_IN"},
  { GfxUIEvent::MOVE_OUT,      "MOVE_OUT"},
  { GfxUIEvent::KEY_PRESS,     "KEY_PRESS"},
  { GfxUIEvent::IDENTIFY,      "IDENTIFY"},
  { GfxUIEvent::DRAG_START,    "DRAG_START"},
  { GfxUIEvent::DRAG_STOP,     "DRAG_STOP"},
  { GfxUIEvent::VALUE_CHANGE,  "VALUE_CHANGE"},
  { GfxUIEvent::INVALID,       "INVALID", (ENUM_WRAPPER_FLAG_CATCHALL)}
};
const EnumDefList<GfxUIEvent> GFXUI_EVENT_LIST(&_ENUM_LIST[0], (sizeof(_ENUM_LIST) / sizeof(_ENUM_LIST[0])));



/*******************************************************************************
* GfxUIElement Base Class
*******************************************************************************/

/* Simple constructor with discrete paramters. */
GfxUIElement::GfxUIElement(PixUInt x, PixUInt y, PixUInt w, PixUInt h, uint32_t f) :
  GfxUILayout(
    x, y, w, h,
    0, 0, 0, 0,
    0, 0, 0, 0
  ),
  _style(),
  _flags(f | GFXUI_FLAG_NEED_RERENDER)
{
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_U)) _bordr_t = 1;   // TODO: Temporary hack until styles are in-use.
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_D)) _bordr_b = 1;   // TODO: Temporary hack until styles are in-use.
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_L)) _bordr_l = 1;   // TODO: Temporary hack until styles are in-use.
  if (_class_flag(GFXUI_FLAG_DRAW_FRAME_R)) _bordr_r = 1;   // TODO: Temporary hack until styles are in-use.
}


/* Constructor with fully-specified paramters. */
GfxUIElement::GfxUIElement(const GfxUILayout layout, const GfxUIStyle style, uint32_t f) :
  GfxUILayout(layout),
  _style(style),
  _flags(f | GFXUI_FLAG_NEED_RERENDER) {}

/* Constructor with fully-specified paramters. */
GfxUIElement::GfxUIElement(GfxUILayout* layout, GfxUIStyle* style, uint32_t f) :
  GfxUILayout(layout),
  _style(style),
  _flags(f | GFXUI_FLAG_NEED_RERENDER) {}



void GfxUIElement::muteRender(bool x) {
  _class_set_flag(GFXUI_FLAG_MUTE_RENDER, x);
  if (!x && (!_class_flag(GFXUI_FLAG_ALWAYS_REDRAW))) {
    // Un-muting the render will cause a redraw, if the class isn't already
    //   going to do it.
    _class_set_flag(GFXUI_FLAG_NEED_RERENDER);
  }
}



void GfxUIElement::reposition(PixUInt x, PixUInt y) {
  int32_t shift_x = x - _x;
  int32_t shift_y = y - _y;
  _x += shift_x;
  _y += shift_y;
  if (_children.hasNext()) {
    // There are child objects to relocate.
    const uint32_t QUEUE_SIZE = _children.size();
    for (uint32_t n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      ui_obj->reposition(ui_obj->elementPosX() + shift_x, ui_obj->elementPosY() + shift_y);
    }
  }
  _need_redraw(true);
}


int GfxUIElement::_add_child(GfxUIElement* chld) {
  const int RET = ((-1 == _children.insert(chld)) ? -1 : 0);
  if (0 == RET) {  _need_redraw(true);  }
  return RET;
}

int GfxUIElement::_remove_child(GfxUIElement* chld) {
  const int RET = (_children.remove(chld) ? 0 : -1);
  if (0 == RET) {  _need_redraw(true);  }
  return RET;
}


bool GfxUIElement::notify(const GfxUIEvent GFX_EVNT, const PixUInt x, const PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  const bool INCLUDES_POINT = includesPoint(x, y);
  _flags.set(GFXUI_FLAG_UNDER_POINTER, INCLUDES_POINT);
  if (INCLUDES_POINT & !muteRender()) {
    switch (GFX_EVNT) {
      // These events we process depth-first, and in the abstraction.
      case GfxUIEvent::HOVER_IN:
      case GfxUIEvent::HOVER_OUT:
      case GfxUIEvent::SELECT:
      case GfxUIEvent::UNSELECT:
      case GfxUIEvent::IDENTIFY:
      case GfxUIEvent::DRAG_START:
      case GfxUIEvent::DRAG_STOP:
        if (!_notify_children(GFX_EVNT, x, y, change_log)) {
          // No children claimed the event. It must be for us.
          switch (GFX_EVNT) {
            case GfxUIEvent::HOVER_IN:
              _pointer_x = x;
              _pointer_y = y;
            case GfxUIEvent::HOVER_OUT:
              // Propagate HOVER to child class.
              isFocused(GFX_EVNT == GfxUIEvent::HOVER_IN);
              //c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, "%p is focused: %c \t underPointer(): %c", this, (isFocused()?'y':'n'), (underPointer()?'y':'n'));
              _notify(GFX_EVNT, x, y, change_log);
              break;
            case GfxUIEvent::SELECT:
            case GfxUIEvent::UNSELECT:
              // Propagate SELECT to child class.
              isSelected((GfxUIEvent::UNSELECT == GFX_EVNT) ^ _notify(GFX_EVNT, x, y, change_log));
              break;

            case GfxUIEvent::DRAG_START:
            case GfxUIEvent::DRAG_STOP:
              if (isDraggable()) {
                // If the element is draggable, move it.
                if (isDragging()) {
                  reposition(x, y);
                }
                else {
                  isDragging(GfxUIEvent::DRAG_START == GFX_EVNT);
                }
              }
              else {
                // If the element isn't draggable, propagate the event.
                _notify(GFX_EVNT, x, y, change_log);
              }
              break;

            default:
              break;
          }
          change_log->insert(this, (int) GFX_EVNT);
          if (GFX_EVNT != GfxUIEvent::IDENTIFY) {
            // Propagate HOVER to child class. We are returning true, in any case.
            _notify(GFX_EVNT, x, y, change_log);
          }
        }
        ret = true;
        break;

      default:
        if (elementActive()) {
          ret = _notify(GFX_EVNT, x, y, change_log);
          if (!ret) {
            ret = _notify_children(GFX_EVNT, x, y, change_log);
          }
        }
        break;
    }
  }
  return ret;
}


bool GfxUIElement::_notify_children(const GfxUIEvent GFX_EVNT, const PixUInt x, const PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  if (_children.hasNext()) {
    // There are child objects to notify.
    const uint32_t QUEUE_SIZE = _children.size();
    for (uint32_t n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      if (ui_obj->notify(GFX_EVNT, x, y, change_log)) {
        return true;
      }
    }
  }
  return false;
}


int GfxUIElement::render(UIGfxWrapper* ui_gfx, bool force) {
  int ret = 0;
  if (!muteRender()) {
    ret += _render_children(ui_gfx, force);
    if (_need_redraw() | force) {
      ret += _render(ui_gfx);
      const uint32_t COLOR = _style.color_border;
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_U) {
        for (uint8_t i = 0; i < _bordr_t; i++) {
          ui_gfx->img()->drawFastHLine(_x, (_y+i), _w, COLOR);
        }
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_D) {
        for (uint8_t i = 0; i < _bordr_b; i++) {
          ui_gfx->img()->drawFastHLine(_x, (_y+(_h-(1+i))), _w, COLOR);
        }
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_L) {
        for (uint8_t i = 0; i < _bordr_l; i++) {
          ui_gfx->img()->drawFastVLine((_x+i), _y, _h, COLOR);
        }
      }
      if (_class_flags() & GFXUI_FLAG_DRAW_FRAME_R) {
        for (uint8_t i = 0; i < _bordr_r; i++) {
          ui_gfx->img()->drawFastVLine((_x+(_w-(1+i))), _y, _h, COLOR);
        }
      }
      _need_redraw(false);
    }
  }
  return ret;
};



int GfxUIElement::_render_children(UIGfxWrapper* ui_gfx, bool force) {
  int ret = 0;
  if (_children.hasNext()) {
    // There are child objects to render.
    const uint32_t QUEUE_SIZE = _children.size();
    for (uint32_t n = 0; n < QUEUE_SIZE; n++) {
      GfxUIElement* ui_obj = _children.get(n);
      ret += ui_obj->render(ui_gfx, force);
    }
  }
  return ret;
}
