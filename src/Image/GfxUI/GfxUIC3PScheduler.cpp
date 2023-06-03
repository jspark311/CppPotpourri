/*
File:   GfxUIC3PScheduler.cpp
Author: J. Ian Lindsay
Date:   2023.05.18

*/

#include "../GfxUI.h"

/*******************************************************************************
* GfxUIC3PScheduler
*******************************************************************************/
/**
* Constructor
* The scheduler is a singleton class, and so we will not require references.
*/
GfxUIC3PScheduler::GfxUIC3PScheduler(const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUITabbedContentPane(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
  _pane_info(0, 0, 0, 0),
  _pane_schedules(0, 0, 0, 0),
  _txt(
    GfxUILayout(
      internalPosX(), (internalPosY() + _tab_bar.elementHeight()),
      internalWidth(), 64,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty
  ),
  _sw_svc_loop(
    "Service Loop",
    &(C3PScheduler::getInstance()->profiler_service),
    GfxUILayout(
      internalPosX(), (_txt.elementPosY() + _txt.elementHeight()),
      internalWidth(), 32,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty
  ),
  _sw_deadband(
    "Deadband",
    &(C3PScheduler::getInstance()->profiler_service),
    GfxUILayout(
      internalPosX(), (_sw_svc_loop.elementPosY() + _sw_svc_loop.elementHeight()),
      internalWidth(), 32,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty
  )
{
  // Note our subordinate objects...
  addTab("Info", &_pane_info, true);
  addTab("Schedules", &_pane_schedules);
  _pane_info.add_child(&_txt);
  _pane_info.add_child(&_sw_svc_loop);
  _pane_info.add_child(&_sw_deadband);
}


int GfxUIC3PScheduler::_render(UIGfxWrapper* ui_gfx) {
  StringBuilder tmp_str;
  C3PScheduler* scheduler = C3PScheduler::getInstance();
  if (scheduler) {
    tmp_str.concatf("Schedule count:   %u\n", scheduler->scheduleCount());
    //tmp_str.concatf("\tLoops (SVC/ISR):  %u / %u\n\n", profiler_service.executions(), _isr_count);
    if (scheduler->scheduleCount() != _dyn_elements.size()) {
      const uint32_t SCH_GUI_HEIGHT = 64;
      uint32_t i_x = internalPosX();
      uint32_t sch_list_y = (internalPosY() + _tab_bar.elementHeight());

      // Remove GUI elements for schedules that no longer exist.
      for (int i = 0; i < _dyn_elements.size(); i++) {
        GfxUIC3PSchedule* gui_element = _dyn_elements.get(i);
        if (!scheduler->containsSchedule(gui_element->getSchedule())) {
          _pane_schedules.remove_child(gui_element);
          _dyn_elements.remove(i);
          delete gui_element;
          i--;
        }
        else {
          // Reposition the element.
          gui_element->reposition(i_x, sch_list_y);
          sch_list_y += SCH_GUI_HEIGHT;
        }
      }
      // Add any new schedules that we don't have GUI elements for yet.
      const unsigned int SCH_COUNT = scheduler->scheduleCount();
      C3PSchedule* sch_ptrs[SCH_COUNT];
      for (unsigned int i = 0; i < SCH_COUNT; i++) {
        sch_ptrs[i] = scheduler->getScheduleByIndex(i);
      }

      for (int i = 0; i < _dyn_elements.size(); i++) {
        C3PSchedule* tst_ptr = _dyn_elements.get(i)->getSchedule();
        for (unsigned int x = 0; x < SCH_COUNT; x++) {
          if (sch_ptrs[x] == tst_ptr) {
            sch_ptrs[x] = nullptr;
            break;
          }
        }
      }

      for (unsigned int i = 0; i < SCH_COUNT; i++) {
        if (nullptr != sch_ptrs[i]) {
          GfxUIC3PSchedule* gui_element = new GfxUIC3PSchedule(
            sch_ptrs[i],
            GfxUILayout(
              i_x, sch_list_y,
              internalWidth(), SCH_GUI_HEIGHT,
              1, 0, 0, 0,   // Margins_px(t, b, l, r)
              0, 1, 0, 0    // Border_px(t, b, l, r)
            ),
            _style,
            (GFXUI_FLAG_FREE_THIS_ELEMENT)
          );
          _dyn_elements.insert(gui_element);
          _pane_schedules.add_child(gui_element);
          sch_list_y += SCH_GUI_HEIGHT;
        }
      }
    }
  }
  else {
    tmp_str.concat("Scheduler not created.\n");
  }
  _txt.clear();
  _txt.provideBuffer(&tmp_str);
  return GfxUITabbedContentPane::_render(ui_gfx);
}


bool GfxUIC3PScheduler::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
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
* GfxUIC3PSchedule
*******************************************************************************/

/**
* Constructor
*/
GfxUIC3PSchedule::GfxUIC3PSchedule(C3PSchedule* schedule, const GfxUILayout lay, const GfxUIStyle sty, uint32_t f) :
  GfxUIElement(lay, sty, (f | GFXUI_FLAG_ALWAYS_REDRAW)),
  _sched(schedule),
  _gfx_profiler(
    schedule->handle(),
    &schedule->profiler,
    GfxUILayout(
      internalPosX(), (internalPosY()+32),
      internalWidth(), 32,
      1, 0, 0, 0,   // Margins_px(t, b, l, r)
      0, 0, 0, 0    // Border_px(t, b, l, r)
    ),
    sty
  )
{
  _add_child(&_gfx_profiler);
};



int GfxUIC3PSchedule::_render(UIGfxWrapper* ui_gfx) {
  int ret = 0;
  C3PScheduler* scheduler = C3PScheduler::getInstance();
  if (scheduler) {
    if (scheduler->containsSchedule(_sched)) {
      uint32_t i_x = internalPosX();
      uint32_t i_y = internalPosY();
      uint16_t i_w = internalWidth();
      uint16_t i_h = internalHeight();

      ui_gfx->img()->setCursor(i_x, i_y);
      ui_gfx->img()->setTextSize(_style.text_size);
      ui_gfx->img()->setTextColor((_sched->enabled() ? _style.color_active : _style.color_inactive), _style.color_bg);
      ui_gfx->img()->writeString(_sched->handle());

      ui_gfx->img()->setCursor((i_x + (i_w >> 1)), i_y);
      ui_gfx->img()->setTextSize(_style.text_size-1);
      const uint32_t SCALE_TXT_H_PIX = ui_gfx->img()->getFontHeight() + 2;

      if (_sched->enabled()) {
        ui_gfx->img()->setTextColor(0x00CC00, _style.color_bg);
        ui_gfx->img()->writeString("Enabled     ");
      }
      else {
        ui_gfx->img()->setTextColor(0x888800, _style.color_bg);
        ui_gfx->img()->writeString("Disabled    ");
      }
      StringBuilder line;
      ui_gfx->img()->setTextColor(0xe0e0e0, _style.color_bg);
      line.concatf("Last executed %u us ago", (micros() - _sched->lastExec()));
      ui_gfx->img()->writeString(&line);
      line.clear();

      ui_gfx->img()->setCursor((i_x + (i_w >> 1)), (i_y + SCALE_TXT_H_PIX));
      line.concatf("Period:     %u us", _sched->period());
      ui_gfx->img()->writeString(&line);
      line.clear();

      ui_gfx->img()->setCursor((i_x + (i_w >> 1)), (i_y + (SCALE_TXT_H_PIX * 2)));
      if (-1 == _sched->recurrence()) {
        line.concat("Recurrence: Forever");
      }
      else {
        line.concatf("Recurrence: %d",    _sched->recurrence());
      }
      ui_gfx->img()->writeString(&line);
      ret = 1;
    }
  }

  return ret;
}



bool GfxUIC3PSchedule::_notify(const GfxUIEvent GFX_EVNT, uint32_t x, uint32_t y, PriorityQueue<GfxUIElement*>* change_log) {
  bool ret = false;
  switch (GFX_EVNT) {
    case GfxUIEvent::TOUCH:
      _sched->enabled(!_sched->enabled());
      ret = true;
      break;

    default:
      break;
  }
  if (ret) {
    change_log->insert(this, (int) GFX_EVNT);
    _need_redraw(true);
  }
  return ret;
}
