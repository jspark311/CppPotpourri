/*
File:   GfxUITabBar.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUITabBar
*******************************************************************************/

int GfxUITabBar::_render(UIGfxWrapper* ui_gfx) {
  int8_t ret = 0;
  const uint32_t BTN_COUNT = (uint32_t) _children.size();
  for (uint32_t btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
    GfxUIButton* btn_cur = (GfxUIButton*) _children.get(btn_idx);
    if (btn_cur->pressed()) {
      if (1 == _set_active_tab(btn_idx)) {
        ret = 1;
      }
    }
  }
  return ret;
}


/*
* GfxUITabBar is a container for buttons with special logic. It does not itself
*   respond to _notify().
*/
bool GfxUITabBar::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {

    case GfxUIEvent::TOUCH:
    case GfxUIEvent::RELEASE:
      if (_children.hasNext()) {
        // There are child objects to notify.
        const uint32_t BTN_COUNT = (uint32_t) _children.size();
        for (uint32_t btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
          GfxUIElement* ui_obj = _children.get(btn_idx);
          if (ui_obj->notify(GFX_EVNT, x, y, change_log)) {
            if (GfxUIEvent::TOUCH == GFX_EVNT) {
              ret = (1 == _set_active_tab(btn_idx));
            }
          }
        }
      }
      break;

    case GfxUIEvent::MOVE_UP:
    case GfxUIEvent::MOVE_DOWN:
      if (scrollCycle()) {
        const uint32_t BTN_COUNT = (uint32_t) _children.size();
        int8_t movement_dir = (GfxUIEvent::MOVE_UP == GFX_EVNT) ? 1 : -1;
        for (uint32_t btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
          GfxUIButton* btn_cur = (GfxUIButton*) _children.get(btn_idx);
          if (btn_cur->pressed()) {
            if (1 == _set_active_tab((btn_idx + movement_dir) % BTN_COUNT)) {
              break;   // Break from the loop.
            }
          }
        }
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


int8_t GfxUITabBar::addTab(const char* txt, bool selected) {
  int8_t ret = -1;
  const uint32_t flgs_inact = 0;
  const uint32_t flgs_act   = (GFXUI_BUTTON_FLAG_STATE | GFXUI_FLAG_INACTIVE);
  const uint32_t BTN_COUNT = (uint32_t) _children.size() + 1;  // Recalculate width.
  const uint32_t INTRNL_X = internalPosX();
  const uint32_t INTRNL_Y = internalPosY();
  const uint32_t INTRNL_H = internalHeight();
  const uint32_t NEW_UNIT_W = (internalWidth()/BTN_COUNT);

  GfxUIButton* n_btn = new GfxUITextButton(
    GfxUILayout(
      INTRNL_X + (NEW_UNIT_W * (BTN_COUNT-1)), INTRNL_Y,
      NEW_UNIT_W, INTRNL_H,
      0, 2, 0, 0,
      0, 0, 0, 0               // Border_px(t, b, l, r)
    ),
    (const GfxUIStyle) _style,
    txt,
    ((GFXUI_FLAG_FREE_THIS_ELEMENT | GFXUI_FLAG_NEED_RERENDER) | (selected ? flgs_act : flgs_inact))
  );
  if (n_btn) {
    ret--;
    if (0 <= _add_child(n_btn)) {
      uint32_t x_pix_accum = 0;
      for (uint32_t btn_idx = 0; btn_idx < (BTN_COUNT-1); btn_idx++) {
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
      for (uint32_t btn_idx = 0; btn_idx < BTN_COUNT; btn_idx++) {
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
* GfxUITabbedContentPane
*******************************************************************************/
GfxUITabbedContentPane::GfxUITabbedContentPane(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _tab_bar(
    GfxUILayout(
      internalPosX(), internalPosY(),
      internalWidth(), ((sty.text_size * 8) + 12),  // TODO: Better, but still arbitrary.
      1, 1, 1, 0,
      0, 1, 0, 0               // Border_px(t, b, l, r)
    ),
    sty,
    (GFXUI_FLAG_DRAW_FRAME_D | GFXUI_TABBAR_FLAG_SCROLL_CYCLES_TABS)
  ),
  _active_tab(0)
{
  // Note our subordinate objects...
  _add_child(&_tab_bar);
}



int GfxUITabbedContentPane::_render(UIGfxWrapper* ui_gfx) {
  int8_t ret = 0;
  if (_active_tab != _tab_bar.activeTab()) {
    ret--;
    _active_tab = _tab_bar.activeTab();
    const uint32_t TAB_COUNT = (uint32_t) _tab_bar.tabCount();
    for (uint32_t i = 0; i < TAB_COUNT; i++) {
      // TODO: Fragile. Assumes tab_bar is the only other direct child, and that it
      //   was added first, and the order never altered.
      GfxUIElement* content = _children.get(i+1);
      if (nullptr != content) {
        if (_active_tab == i) {
          content->elementActive(true);
          content->muteRender(false);
          content->fill(ui_gfx, _style.color_bg);
          content->render(ui_gfx, true);  // Force a re-render of the newly-active tab content.
        }
        else {
          // Set all other tabs inactive, and insensitive.
          content->elementActive(false);
          content->muteRender(true);
        }
      }
      ret = 1;
    }
  }
  return ret;
}


bool GfxUITabbedContentPane::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  // If the event was directed at the nav pane, it will be handled by the natural
  //   flow that comes from its addition as a child object.
  return false;
}


int8_t GfxUITabbedContentPane::addTab(const char* txt, GfxUIElement* content, bool selected) {
  int8_t ret = -1;
  if (0 == _tab_bar.addTab(txt, selected)) {
    const uint32_t INTRNL_X = internalPosX();
    const uint32_t INTRNL_Y = internalPosY() + _tab_bar.elementHeight();
    const uint32_t INTRNL_W = internalWidth();
    const uint32_t INTRNL_H = internalHeight() - _tab_bar.elementHeight();
    content->reposition(INTRNL_X, INTRNL_Y);
    content->resize(INTRNL_W, INTRNL_H);
    content->elementActive(selected);
    content->muteRender(!selected);
    _add_child(content);
    ret = 0;
  }
  return ret;
}
