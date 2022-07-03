/*
File:   GfxUITabBar.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUITabBar
*******************************************************************************/
GfxUITabBar::GfxUITabBar(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, f), _color(color), _active_tab(0) {}


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
* GfxUITabBarWithContent
*******************************************************************************/
GfxUITabBarWithContent::GfxUITabBarWithContent(uint32_t x, uint32_t y, uint16_t w, uint16_t h, uint32_t color, uint32_t f) :
  GfxUIElement(x, y, w, h, f),
  _tab_bar(_internal_PosX(), _internal_PosY(), _internal_Width(), 20, color, GFXUI_FLAG_DRAW_FRAME_D),
  _active_tab(0)
{
  // Note our subordinate objects...
  _add_child(&_tab_bar);
}


int GfxUITabBarWithContent::_render(UIGfxWrapper* ui_gfx) {
  int8_t ret = 0;
  const uint32_t TAB_COUNT = (uint32_t) _tab_bar.tabCount();
  for (uint i = 0; i < TAB_COUNT; i++) {
    if (i == _tab_bar.activeTab()) {
      ret = _set_active_tab(i);
    }
  }
  return ret;
}


bool GfxUITabBarWithContent::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y) {
  // If the event was directed at the nav pane, it will be handled by the natural
  //   flow that comes from its addition as a child object.
  bool ret = false;
  return ret;
}


int8_t GfxUITabBarWithContent::addTab(const char* txt, GfxUIElement* content, bool selected) {
  int8_t ret = -1;
  if (0 == _tab_bar.addTab(txt, selected)) {
    const uint32_t INTRNL_X = _internal_PosX();
    const uint32_t INTRNL_Y = _internal_PosY() + 20;
    const uint32_t INTRNL_W = _internal_Width();
    const uint32_t INTRNL_H = _internal_Height() - 20;
    content->reposition(INTRNL_X, INTRNL_Y);
    content->resize(INTRNL_W, INTRNL_H);
    content->elementActive(selected);
    content->muteRender(!selected);
    _add_child(content);
    ret = 0;
  }
  return ret;
}


int8_t GfxUITabBarWithContent::_set_active_tab(uint8_t tab_idx) {
  int8_t ret = 0;
  if (_active_tab != tab_idx) {
    ret--;
    const uint32_t TAB_COUNT = (uint32_t) _tab_bar.tabCount();
    for (uint i = 0; i < TAB_COUNT; i++) {
      // TODO: Fragile. Assumes tab_bar is the only other direct child, and that it
      //   was added first, and the order never altered.
      GfxUIElement* content = _children.get(i+1);
      if (nullptr != content) {
        if (tab_idx == i) {
          content->elementActive(true);
          content->muteRender(false);
        }
        else {
          // Set all other tabs inactive, and insensitive.
          content->elementActive(false);
          content->muteRender(true);
        }
      }
      _active_tab = tab_idx;
      ret = 1;
    }
  }
  return ret;
}
