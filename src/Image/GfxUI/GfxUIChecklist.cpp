/*
File:   GfxUIChecklist.cpp
Author: J. Ian Lindsay
Date:   2024.06.22

*/

#include "../GfxUI.h"



/*******************************************************************************
* GfxUIChecklist
*******************************************************************************/
GfxUIChecklist::GfxUIChecklist(AsyncSequencer* CHK_LIST, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, f),
  _CHK_LIST(CHK_LIST) {};


int GfxUIChecklist::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  {
    const PixUInt I_X = internalPosX();
    const PixUInt I_Y = internalPosY();
    const PixUInt I_W = internalWidth();
    const PixUInt I_H = internalHeight();
    ui_gfx->img()->fillRect(I_X, I_Y, I_W, I_H, _style.color_bg);
    ui_gfx->img()->setTextSize(_style.text_size);
    const PixUInt TXT_PIXEL_WIDTH  = ui_gfx->img()->getFontWidth();
    const PixUInt TXT_PIXEL_HEIGHT = ui_gfx->img()->getFontHeight();
    const PixUInt LINE_H_DELTA = (TXT_PIXEL_HEIGHT + _style.text_size);

    //StringBuilder step_list;
    //const uint32_t STEP_COUNT = _CHK_LIST->stepList(&step_list);
    const uint32_t STEP_COUNT = _CHK_LIST->stepCount();
    uint32_t max_label_len = 0;

    for (uint32_t i = 0; i < STEP_COUNT; i++) {
      const StepSequenceList* STEP = _CHK_LIST->getStep(i);
      const uint32_t LABEL_LEN = strlen(STEP->LABEL);
      max_label_len = strict_max(max_label_len, LABEL_LEN);
    }
    const PixUInt LABEL_COL_WIDTH = ((max_label_len + 1) * TXT_PIXEL_WIDTH);
    PixUInt HEIGHT_FROM_OPTS      = (TXT_PIXEL_HEIGHT * (_class_flag(GFXUI_CHKLST_FLAG_SHOW_SUMMARY) ? 2 : 0));
    PixUInt COL_WIDTH_FROM_OPTS   = (TXT_PIXEL_WIDTH * (_class_flag(GFXUI_CHKLST_FLAG_SHOW_DEPS_MASKS) ? 22 : 0));
    COL_WIDTH_FROM_OPTS += (TXT_PIXEL_WIDTH * (_class_flag(GFXUI_CHKLST_FLAG_SHOW_EXPLICIT_STATE) ? 12 : 0));
    // Now, re-run the loop with render params solved.
    for (uint32_t i = 0; i < STEP_COUNT; i++) {
      const StepSequenceList* STEP = _CHK_LIST->getStep(i);
      const uint32_t STEP_FLAG = STEP->FLAG;
      const PixUInt THIS_LINE_Y = (I_Y + (i * LINE_H_DELTA));
      uint32_t label_color = _style.color_inactive;  // Inactive color by default.
      ui_gfx->img()->setCursor(I_X, THIS_LINE_Y);
      if (_CHK_LIST->all_steps_have_run(STEP_FLAG)) {
        // Green for PASS. Red for FAIL.
        label_color = (_CHK_LIST->all_steps_have_passed(STEP_FLAG)) ? 0x00FF00 : 0xFF1010;
      }
      else if (_CHK_LIST->all_steps_still_running(STEP_FLAG)) {
        label_color = 0xC0C000;   // Yellow for Running.
      }
      else if (_CHK_LIST->all_steps_dispatched(STEP_FLAG)) {
        label_color = _style.color_active;  // active color for Requested.
      }
      ui_gfx->img()->setTextColor(label_color, _style.color_bg);
      ui_gfx->img()->writeString(STEP->LABEL);
      if (_class_flag(GFXUI_CHKLST_FLAG_SHOW_DEPS_MASKS)) {
        ui_gfx->img()->setCursor((I_X + LABEL_COL_WIDTH), THIS_LINE_Y);
        StringBuilder tmp_sb;
        ui_gfx->img()->setTextColor(_style.color_selected, _style.color_bg);
        tmp_sb.concatf("0x%08x 0x%08x", STEP_FLAG, STEP->DEP_MASK);
        ui_gfx->img()->writeString(&tmp_sb);
      }

      if (_class_flag(GFXUI_CHKLST_FLAG_SHOW_EXPLICIT_STATE)) {
        ui_gfx->img()->setTextColor(label_color, _style.color_bg);
        if (_CHK_LIST->all_steps_have_run(STEP_FLAG)) {
          ui_gfx->img()->writeString(_CHK_LIST->all_steps_have_passed(STEP_FLAG) ? "PASS" : "FAIL");
        }
        else if (_CHK_LIST->all_steps_still_running(STEP_FLAG)) {
          ui_gfx->img()->writeString("Running");
        }
        else if (_CHK_LIST->all_steps_dispatched(STEP_FLAG)) {
          ui_gfx->img()->writeString("Requested");
        }
      }
    }
    if (_class_flag(GFXUI_CHKLST_FLAG_SHOW_SUMMARY)) {
      const PixUInt THIS_LINE_Y = I_Y + (STEP_COUNT * LINE_H_DELTA);
      StringBuilder tmp_sb;
      uint32_t label_color = _style.color_inactive;
      ui_gfx->img()->setCursor(I_X, (THIS_LINE_Y + 4));
      if (_CHK_LIST->request_completed()) {
        // Green for PASS. Red for FAIL.
        const bool REQ_FULFILLED = _CHK_LIST->request_fulfilled();
        label_color = (REQ_FULFILLED ? 0x00FF00 : 0xFF0000);
        if (!REQ_FULFILLED) {
          tmp_sb.concatf("%u failures", _CHK_LIST->failed_steps(true));
        }
        else {
          tmp_sb.concat("PASS");
        }
      }
      else if (_CHK_LIST->steps_running()) {
        label_color = 0xC0C000;   // Yellow for Running.
        tmp_sb.concat("Running");
      }
      else {
        tmp_sb.concat("Inactive");
      }
      ui_gfx->img()->setTextColor(label_color, _style.color_bg);
      ui_gfx->img()->writeString(&tmp_sb);
      ui_gfx->img()->drawFastHLine(I_X, THIS_LINE_Y, (LABEL_COL_WIDTH + COL_WIDTH_FROM_OPTS), label_color);
    }

    if (_class_flag(GFXUI_FLAG_AUTOSCALE_ON_REDRAW)) {
      PixUInt PEAK_WIDTH  = (LABEL_COL_WIDTH + COL_WIDTH_FROM_OPTS);
      PixUInt PEAK_HEIGHT = (STEP_COUNT * LINE_H_DELTA) + HEIGHT_FROM_OPTS;
      resize(PEAK_WIDTH, PEAK_HEIGHT);
      _class_clear_flag(GFXUI_FLAG_AUTOSCALE_ON_REDRAW);
    }
    ret = 1;
  }
  return ret;
}


bool GfxUIChecklist::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  return false;
}


/*******************************************************************************
* GfxUIInteractiveChecklist
*******************************************************************************/
GfxUIInteractiveChecklist::GfxUIInteractiveChecklist(AsyncSequencer* CHK_LIST, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIChecklist(CHK_LIST, lay, sty, (f | GFXUI_FLAG_AUTOSCALE_ON_REDRAW)) {};


int GfxUIInteractiveChecklist::_render(UIGfxWrapper* ui_gfx) {
  int ret = GfxUIChecklist::_render(ui_gfx);
  return ret;
}


bool GfxUIInteractiveChecklist::_notify(const GfxUIEvent GFX_EVNT, PixUInt x, PixUInt y, PriorityQueue<GfxUIElement*>* change_log) {
  return false;
}
