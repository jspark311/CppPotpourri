/*
File:   GfxUISlider.cpp
Author: J. Ian Lindsay
Date:   2022.06.25

*/

#include "../GfxUI.h"


/*******************************************************************************
* GfxUISlider
*******************************************************************************/

int GfxUISlider::_render(UIGfxWrapper* ui_gfx) {
  uint32_t i_x = internalPosX();
  uint32_t i_y = internalPosY();
  uint16_t i_w = internalWidth();
  uint16_t i_h = internalHeight();
  if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
    ui_gfx->drawProgressBarV(
      i_x, i_y, i_w, i_h, _style.color_active,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  else {
    ui_gfx->drawProgressBarH(
      i_x, i_y, i_w, i_h, _style.color_active,
      true, _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _percentage
    );
  }
  return 1;
}


bool GfxUISlider::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  float tmp_percentage = PI;  // Something clearly too big to be valid.

  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      change_log->insert(this, (int) GfxUIEvent::DRAG_START);
      // NOTE: No break;
    case GfxUIEvent::DRAG_START:
      if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
        const float PIX_POS_REL = y - internalPosY();
        tmp_percentage = 1.0f - strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float) internalHeight())));
      }
      else {
        const float PIX_POS_REL = x - internalPosX();
        tmp_percentage = strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float) internalWidth())));
      }
      ret = true;
      break;

    case GfxUIEvent::RELEASE:
      change_log->insert(this, (int) GfxUIEvent::DRAG_STOP);
      ret = true;
      break;

    case GfxUIEvent::MOVE_UP:
      tmp_percentage = strict_min(1.0, (_percentage + 0.01));
      change_log->insert(this, (int) GFX_EVNT);
      ret = true;
      break;

    case GfxUIEvent::MOVE_DOWN:
      tmp_percentage = strict_max(0.0, (_percentage - 0.01));
      change_log->insert(this, (int) GFX_EVNT);
      ret = true;
      break;

    default:
      return false;
  }

  if ((1.0 >= tmp_percentage) && (_percentage != tmp_percentage)) {
    _percentage = tmp_percentage;
    change_log->insert(this, (int) GfxUIEvent::VALUE_CHANGE);
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}



/*******************************************************************************
* GfxUIZoomSlider
*******************************************************************************/
float GfxUIZoomSlider::value() {
  return ((_frac_0 + _frac_1) / 2.0f);
}


void GfxUIZoomSlider::value(const float NEW_CENTER) {
  const float WIDTH       = markWidth();
  const float HALF_WIDTH  = (WIDTH / 2.0f);
  const float TMP_FRAC    = 1.0f - strict_min(1.0f, strict_max(0.0f, NEW_CENTER));
  const float RESIDUE_0   = strict_max(0.0, (TMP_FRAC - HALF_WIDTH));
  const float RESIDUE_1   = strict_min(1.0, (TMP_FRAC + HALF_WIDTH));
  const float PROJ_ANTIPODE_0 = (RESIDUE_1 - WIDTH);
  const float PROJ_ANTIPODE_1 = (RESIDUE_0 + WIDTH);
  _frac_0 = strict_min(RESIDUE_0, PROJ_ANTIPODE_0);
  _frac_1 = strict_max(RESIDUE_1, PROJ_ANTIPODE_1);
}


void GfxUIZoomSlider::value(const float M0, const float M1) {
  _frac_0 = strict_min(M0, M1);
  _frac_1 = strict_max(M0, M1);
}


int GfxUIZoomSlider::_render(UIGfxWrapper* ui_gfx) {
  uint32_t i_x = internalPosX();
  uint32_t i_y = internalPosY();
  uint16_t i_w = internalWidth();
  uint16_t i_h = internalHeight();
  if (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL)) {
    ui_gfx->drawZoomBarV(
      i_x, i_y, i_w, i_h, _style.color_active,
      _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _frac_0, _frac_1
    );
  }
  else {
    ui_gfx->drawZoomBarH(
      i_x, i_y, i_w, i_h, _style.color_active,
      _class_flag(GFXUI_SLIDER_FLAG_RENDER_VALUE),
      _frac_0, _frac_1
    );
  }
  return 1;
}


bool GfxUIZoomSlider::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  float tmp_frac_0 = PI;  // Something clearly too big to be valid.
  float tmp_frac_1 = PI;  // Something clearly too big to be valid.

  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      change_log->insert(this, (int) GfxUIEvent::DRAG_START);
      // NOTE: No break;
    case GfxUIEvent::DRAG_START:
      {
        const PixUInt FIELD_WIDTH_PX = (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL) ? internalHeight() : internalWidth());
        const float   PIX_POS_REL    = (_class_flag(GFXUI_SLIDER_FLAG_VERTICAL) ? (y - internalPosY()) : (x - internalPosX()));
        const float TMP_FRAC    = 1.0f - strict_min(1.0f, strict_max(0.0f, (PIX_POS_REL / (float) FIELD_WIDTH_PX)));
        const float WIDTH       = markWidth();
        const float HALF_WIDTH  = (WIDTH / 2.0f);
        const float RESIDUE_0   = strict_max(0.0, (TMP_FRAC - HALF_WIDTH));
        const float RESIDUE_1   = strict_min(1.0, (TMP_FRAC + HALF_WIDTH));
        const float PROJ_ANTIPODE_0 = (RESIDUE_1 - WIDTH);
        const float PROJ_ANTIPODE_1 = (RESIDUE_0 + WIDTH);
        tmp_frac_0 = strict_min(RESIDUE_0, PROJ_ANTIPODE_0);
        tmp_frac_1 = strict_max(RESIDUE_1, PROJ_ANTIPODE_1);
      }
      ret = true;
      break;

    case GfxUIEvent::RELEASE:
      change_log->insert(this, (int) GfxUIEvent::DRAG_STOP);
      ret = true;
      break;

    case GfxUIEvent::MOVE_UP:
      // Scrolling down will increase the righthand bound, and decrease the lefthand bound.
      tmp_frac_0 = strict_max(0.0, (_frac_0 - 0.005));
      tmp_frac_1 = strict_min(1.0, (_frac_1 + 0.005));
      change_log->insert(this, (int) GFX_EVNT);
      ret = true;
      break;

    case GfxUIEvent::MOVE_DOWN:
      // Scrolling up will decrease the righthand bound, and increase the lefthand bound.
      tmp_frac_0 = strict_min(1.0, (_frac_0 + 0.005));
      tmp_frac_1 = strict_max(0.0, (_frac_1 - 0.005));
      change_log->insert(this, (int) GFX_EVNT);
      ret = true;
      break;

    default:
      return false;
  }

  const bool CHANGE_REGISTERED = ((1.0 >= tmp_frac_0) && (1.0 >= tmp_frac_1));
  const bool CHANGE_NONTRIVIAL = (CHANGE_REGISTERED && ((_frac_0 != tmp_frac_0) || (_frac_1 != tmp_frac_1)));
  if (CHANGE_NONTRIVIAL) {
    _frac_0 = tmp_frac_0;
    _frac_1 = tmp_frac_1;
    change_log->insert(this, (int) GfxUIEvent::VALUE_CHANGE);
  }
  if (ret) {
    _need_redraw(true);
  }
  return ret;
}
